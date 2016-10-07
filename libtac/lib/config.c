
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "xalloc.h"

#define xstr(s) str(s)
#define str(s) #s
#ifndef PATH_TAC_CLIENT_CONF
#define PATH_TAC_CLIENT_CONF  "/etc/config/tac_client.conf"
#endif

struct tac_config * tac_config_load(void)
{
    struct tac_config *config;

    config = _tac_config_readfile(xstr(PATH_TAC_CLIENT_CONF));
    if (config) {
        _tac_config_apply(config);
    }
    return config;
}

/*
 * input is a string like [::1]:123, which will be mangled into [::1]\0  and
 * port will be set to point to the port section of the input string, iff found.
 */
int split_port_ipv6(char *input, char **port)
{
        char *open_sb, *close_sb, *port_delim;

        open_sb = strchr(input, '[');
        if (!open_sb) {
                return 1; /* not a valid ipv6 address. */
        }

        /* check for matching ']' */
        close_sb = strchr(open_sb, ']');
        if (!close_sb) {
                return 1; /* not a valid ipv6 address. */
        }

        /* mangle input; terminate input string after address */
        close_sb++;
        *close_sb = '\0';

        /* look for a port */
        port_delim = strchr(close_sb, ':');
        if (port_delim && strlen(input) > (port_delim - open_sb)) {
                /* delimiter found AND there is something after it. */
                *port = port_delim + 1;
        }
        /* else: no port delimiter, or no port. leave *port untouched. */
        return 0;
}

/*
 * input is a string like 1.2.3.4:567, which will be mangled into 1.2.3.4\0 and
 * port will be set to point to the port section of the input string, iff found.
 */
int split_port(char *input, char **port)
{
        char *delim;

        /* first try ipv6 */
        if (0 == split_port_ipv6(input, port)) {
                return 0; /* success! */
        }

        /* else try ipv4 or hostname */
        delim = strchr(input, ':');

        if (delim && delim != input) {
                /* input like "1.2.3.4:567" or "1.2.3.4:" */
                if (strlen(input) > (delim - input + 1)) {
                        /* there's something after the delimiter */
                        *port = delim + 1;
                }
                /* else: there is nothing after the ':' so leave port alone */

                *delim = '\0'; /* terminate input string at delimiter */
        } else if (delim == input) {
                // no server specified. input like ":123"
                return 1;
        }
        // else: input like "1.2.3.4", leave it alone.
        return 0;
}

struct tac_config * _tac_config_readfile(const char *path)
{
    FILE *f;
    int lineno = 0;
    struct tac_config *ret;
    char line[1024];
    char *default_secret = NULL;
    unsigned int nservers = 0;

    f = fopen(path, "r");
    if (!f) {
	TACSYSLOG((LOG_ERR, "%s: %m", path));
        return NULL;
    }

    ret = xcalloc(1, sizeof *ret);
    ret->path = xstrdup(path);
    while (fgets(line, sizeof line, f) != NULL) {
	int len;
	char *toksav, *key, *value;

        ++lineno;
	/* Remove trailing \n */
	len = strlen(line);
	if (len > 0 && line[len - 1] == '\n') {
	    line[--len] = '\0';
	}
	/* Ignore comment lines and blank lines */
	if (line[0] == '#' || line[0] == '\0') {
	    continue;
	}
	/* Lines are of the form "key [=] value" */
	key = strtok_r(line, " =\t", &toksav);
	value = strtok_r(NULL, " \t", &toksav);
	if (!key) {
	    TACSYSLOG((LOG_WARNING, "%s:%d: bad line", path, lineno));
	    continue;
	}

	/*
	 * Treat 'server' and 'key' (secret) lines carefully.
	 * Historically, libtacplus only understood a single 'key' line, and it
	 * didn't matter if it came before or after the (multiple) server
	 * lines. However, here we allow a 'key' line to set the secret for all
	 * previous server line(s) without keys. Also, if a 'key' line is found
	 * early (before all 'server'), then that secret is used for all
	 * subsequent servers not followed by an explicit key.
	 */
	if (strcmp(key, "key") == 0) {
	    if (nservers == 0) {
		free(default_secret);
		default_secret = xstrdup(value);
	    } else {
	        unsigned i;
		free(ret->server[nservers - 1].secret);
		ret->server[nservers - 1].secret = NULL;
		i = nservers - 1;
		do {
		    if (!ret->server[i].secret) {
		        ret->server[i].secret = xstrdup(value);
		    }
		} while (i--);
	    }
	}
	else if (strcmp(key, "server") == 0) {
	    struct tac_config_server *server;
	    ret->nservers++;
	    ret->server = xrealloc(ret->server, ret->nservers * sizeof *ret->server);
	    server = &ret->server[ret->nservers - 1];
	    server->name = xstrdup(value);
	    split_port(server->name, &server->service);
	    server->secret = xstrdup(default_secret);
	    server->lineno = lineno;
	}
	else {
	    /* Add all other key[=value] to the config[] list */
	    struct tac_config_config *config;
	    ret->nconfig++;
	    ret->config = xrealloc(ret->config, ret->nconfig * sizeof *ret->config);
	    config = &ret->config[ret->nconfig - 1];
	    config->key = xstrdup(key);
	    config->value = xstrdup(value);
	    config->lineno = lineno;
	}
    }
    return ret;
}

void  _tac_config_apply(const struct tac_config *config)
{
    const struct tac_config_config *c;

    xstrcpy(tac_login, "", sizeof tac_login);

    if (!config) {
	return;
    }

    for (c = config->config; c < config->config + config->nconfig; ++c) {
	if (strcmp(c->key, "login") == 0) {
            xstrcpy(tac_login, c->value, sizeof tac_login);
	}
    }
}

void tac_config_free(struct tac_config *config)
{
    if (config) {
        unsigned i;
	for (i = 0; i < config->nservers; i++) {
	    free(config->server[i].name);
	    free(config->server[i].service);
	    free(config->server[i].secret);
	}
        free(config->server);
	for (i = 0; i < config->nconfig; i++) {
	    free(config->config[i].key);
	    free(config->config[i].value);
	}
        free(config->config);
        free(config->path);
	free(config);
    }
}
