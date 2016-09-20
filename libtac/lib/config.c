
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "xalloc.h"

#define SYSCONFDIR "/etc/config"
#define PATH_TAC_CLIENT_CONF  SYSCONFDIR "/tac_client.conf"

struct tac_config * tac_config_load(void)
{
    struct tac_config *config;

    config = _tac_config_readfile(PATH_TAC_CLIENT_CONF);
    if (config) {
        _tac_config_apply(config);
    }
    return config;
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
	    server->service = xstrdup("49");
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
