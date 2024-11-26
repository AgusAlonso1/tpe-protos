#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <errno.h>
#include <getopt.h>

#include <args.h>

static void parse_args(const int argc, char **argv, pop3args *args);

static pop3args pop3_args;

void init_server_args(int argc, char **argv) {
    parse_args(argc, argv, &pop3_args);
}

bool exists_user(char * name, char * pass) {
    for (int i = 0; i < pop3_args.users_len; i++) {
        if ((strcmp(name, pop3_args.users[i].name) == 0) && (strcmp(pass, pop3_args.users[i].pass) == 0)) {
            return true;
        }
    }
    return false;
}

char * get_transformation_bin() {
    return pop3_args.tranformation_bin;
}

char * get_mail_dirs_path() {
    return pop3_args.mail_dirs_path;
}

unsigned short get_pop3_port() {
    return pop3_args.pop3_port;
}

char * get_pop3_addr() {
    return pop3_args.pop3_addr;
}

unsigned short get_manager_port() {
    return pop3_args.mng_port;
}

char * get_manager_addr() {
    return pop3_args.mng_addr;
}

static unsigned short
port(const char *s) {
     char *end     = 0;
     const long sl = strtol(s, &end, 10);

     if (end == s|| '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 0 || sl > USHRT_MAX) {
         fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
         exit(1);
         return 1;
     }
     return (unsigned short)sl;
}

static void
usr(char *s, struct user *usr) {
    char *p = strchr(s, ':');
    if(p == NULL) {
        fprintf(stderr, "password not found\n");
        exit(1);
    } else {
        *p = 0;
        p++;
        usr->name = s;
        usr->pass = p;
    }

}

static void
version(void) {
    fprintf(stderr, "pop3 version 1.0\n"
                    "ITBA Protocolos de Comunicación 2024/2 -- Grupo 8\n"
                    "AQUI VA LA LICENCIA\n");
}

static void
usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s [OPTION]...\n"
        "\n"
        "   -h               Imprime la ayuda y termina.\n"
        "   -l <POP3 addr>   Dirección donde servirá el servicio POP3.\n"
        "   -L <mng  addr>   Dirección donde servirá el servicio de management.\n"
        "   -p <POP3 port>   Puerto entrante conexiones POP3.\n"
        "   -P <mng port>    Puerto entrante servicio de management.\n"
        "   -u <name>:<pass> Usuario y contraseña de usuario que puede usar el servidor. Hasta 10.\n"
        "   -v               Imprime información sobre la versión versión y termina.\n"
        "   -d <dir>         Carpeta donde residen los Maildirs.\n"
        "   -t <cmd>         Comando para aplicar transformaciones.\n"
        "\n",
        progname);
    exit(1);
}

static void 
parse_args(const int argc, char **argv, pop3args *args) {
    memset(args, 0, sizeof(*args)); // sobre todo para setear en null los punteros de users

    args->pop3_addr = "0.0.0.0";
    args->pop3_port = 8085;

    args->mng_addr   = "127.0.0.1";
    args->mng_port   = 8086;

    args->mail_dirs_path = NULL;
    args->tranformation_bin = "cat";

    int c;
    int nusers = 0;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            { "doh-ip",    required_argument, 0, 0xD001 },
            { "doh-port",  required_argument, 0, 0xD002 },
            { "doh-host",  required_argument, 0, 0xD003 },
            { "doh-path",  required_argument, 0, 0xD004 },
            { "doh-query", required_argument, 0, 0xD005 },
            { 0,           0,                 0, 0 }
        };

        c = getopt_long(argc, argv, "hl:L:Np:P:u:v:t:d:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'l':
                args->pop3_addr = optarg;
                break;
            case 'L':
                args->mng_addr = optarg;
                break;
            case 'd':
                args->mail_dirs_path = optarg;
                break;
            case 'p':
                args->pop3_port = port(optarg);
                break;
            case 'P':
                args->mng_port   = port(optarg);
                break;
            case 'u':
                if(nusers >= MAX_USERS) {
                    fprintf(stderr, "maximun number of command line users reached: %d.\n", MAX_USERS);
                    exit(1);
                } else {
                    usr(optarg, args->users + nusers);
                    nusers++;
                }
                break;
            case 'v':
                version();
                exit(0);
                break;
            case 't':
                args->tranformation_bin = optarg;
                break;
            default:
                fprintf(stderr, "unknown argument %d.\n", c);
                exit(1);
        }

    }
    args->users_len = nusers;

    if (optind < argc) {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
}
