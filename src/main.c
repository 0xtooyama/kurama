#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>

// tags
#define A_TAG   1
#define IMG_TAG 2

// attrs
#define HREF_ATTR 1
#define SRC_ATTR  2

struct option long_options[] = {
    {"url", required_argument, NULL, (int)'u'},
    {"savedir", required_argument, NULL, (int)'s'},
    {"fmt", required_argument, NULL, (int)'f'},
    {"mainpage", required_argument, NULL, (int)'m'},
    {"logfile", required_argument, NULL, (int)'l'},
    {"tag", required_argument, NULL, (int)'t'},
    {"seq_name", required_argument, NULL, (int)'1'}
};

typedef struct {
    char *url;
    int urllen;
    char *savedir;
    char *targetfmt;
    char *mainpage_name;
    char *wget_logfile;
    int targettag;
    char *sequential_save;

    char *links_file;
} opts_t;

typedef struct atag_t {
    char *href;
    struct atag_t *next;
} atag_t;

typedef struct imgtag_t {
    char *src;
    struct imgtag_t *next;
} imgtag_t;

typedef struct {
    atag_t *atags;
    imgtag_t *imgtags;
} mainpage_t;

typedef struct link_t {
    char *value;
    struct link_t *next;
} link_t;

opts_t * parse_opts(int argc, char **argv) {
    opts_t *opts = (opts_t *)calloc(1, sizeof(opts_t));
    if (!opts) {
        fprintf(stderr, "opts calloc error\n");
        exit(1);
    }

    int opt = 0;
    while ((opt = getopt_long(argc, argv, "u:s:f:m:l:t:", long_options, NULL)) != -1) {
        char *opt_value = (char *)calloc(strlen(optarg) + 1, sizeof(char));
        if (!opt_value) {
            fprintf(stderr, "opt_value calloc error\n");
            //free_opts();
            exit(1);
        }

        strcpy(opt_value, optarg);

        switch (opt) {
            case 'u': 
                opts->url = opt_value;
                opts->urllen = strlen(opt_value);
                break;
            case 's':
                opts->savedir = opt_value;
                break;
            case 'f':
                opts->targetfmt = opt_value;
                break;
            case 'm':
                opts->mainpage_name = opt_value;
                break;
            case 'l':
                opts->wget_logfile = opt_value;
                break;
            case 't':
                if (!strcmp(opt_value, "a")) opts->targettag = A_TAG;
                else if (!strcmp(opt_value, "img")) opts->targettag = IMG_TAG;
                break;
            case '1':
                if (opt_value[0] == '.') 
                    opts->sequential_save = &opt_value[1];
                else 
                    opts->sequential_save = opt_value;
                break;
            default :
                // free_opts(opts);
                exit(1);
        }
    }

    if (argc > optind && opts->urllen == 0) {
        opts->url = (char *)calloc(strlen(argv[optind]) + 1, sizeof(char));
        if (!opts->url) {
            fprintf(stderr, "opts->url calloc error\n");
            //free_opts();
            exit(1);
        }
        strcpy(opts->url, argv[optind]);
        opts->urllen = strlen(argv[optind]);
    }

    return opts;
}

char * get_now() {
    time_t t = time(NULL);
    if (t == -1) {
        fprintf(stderr, "get time erorr\n");
        // free_opts()
        exit(1);
    }
    struct tm *tm = localtime(&t);
    if (tm == NULL) {
        fprintf(stderr, "get localtime erorr\n");
        // free_opts()
        exit(1);
    }
    char *now = (char *)calloc(strlen("yyyyMMddhhmmss") + 1, sizeof (char));
    if (!now) {
        fprintf(stderr, "now calloc error\n");
        //free_opts();
        exit(1);
    }
    sprintf(now, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday + 1, tm->tm_hour, tm->tm_min, tm->tm_sec);
    return now;
}

void default_opts(opts_t *opts) {
    if (opts->savedir == NULL) {
        opts->savedir = get_now();
    }

    if (opts->mainpage_name == NULL) {
        char *mainpage_name = "main.html";
        opts->mainpage_name = (char *)calloc(strlen(mainpage_name) + 1, sizeof (char));
        if (!opts->mainpage_name) {
            fprintf(stderr, "opts->mainpage_name calloc error\n");
            //free_opts();
            exit(1);
        }
        strcpy(opts->mainpage_name, mainpage_name);
    }

    if (opts->wget_logfile == NULL) {
        char *wget_logfile = "wget_log.txt";
        opts->wget_logfile = (char *)calloc(strlen(wget_logfile) + 1, sizeof (char));
        if (!opts->wget_logfile) {
            fprintf(stderr, "opts->wget_logfile calloc error\n");
            //free_opts();
            exit(1);
        }
        strcpy(opts->wget_logfile, wget_logfile);
    }

    if (opts->links_file == NULL) {
        char *links_file = "links.txt";
        opts->links_file = (char *)calloc(strlen(links_file) + 1, sizeof (char));
        if (!opts->links_file) {
            fprintf(stderr, "opts->links_file calloc error\n");
            //free_opts();
            exit(1);
        }
        strcpy(opts->links_file, links_file);
    }

    if (opts->targettag == 0) opts->targettag = A_TAG;
    if (opts->sequential_save == 0) opts->sequential_save = 0;
}

void mk_savedir(opts_t *opts) {
    char *savedir = opts->savedir;
    DIR *savedirp = opendir(savedir);
    if (savedirp) {
        fprintf(stderr, "directory %s is existed.\n", savedir);
        closedir(savedirp);
        //free_opts();
        exit(1);
    }

    int rc = mkdir(savedir, 0755);
    if (rc == -1) {
        fprintf(stderr, "mkdir %s error\n", savedir);
        exit(1);
    }
}

void cd_savedir(opts_t *opts) {
    int rc = chdir(opts->savedir);
    if (rc == -1) {
        fprintf(stderr, "chdir %s failed.\n", opts->savedir);
        //free_opts()
        exit(1);
    }
}

void wget_mainpage(opts_t *opts) {
    char wgetcmd[2048];
    sprintf(wgetcmd, "wget -o %s -O %s %s", opts->wget_logfile, opts->mainpage_name, opts->url);
    printf("[wget command] %s\n", wgetcmd);
    system(wgetcmd);
    FILE *mainpagep = fopen(opts->mainpage_name, "r");
    if (!mainpagep) {
        fprintf(stderr, "wget mainpage failed.\n");
        //free_opts()
        exit(1);
    }
    fclose(mainpagep);
}

char * get_tagattr(FILE *mainpagep, char *attr) {
    char tag_values[2048] = "";
    int tag_value_len = 0;
    char c;
    for (int i = 0; i < 2048; i++) {
        c = fgetc(mainpagep);
        if (c == '>') {
            tag_value_len = i;
            break;
        }
        tag_values[i] = c;
    }

    char *href_begin = strstr(tag_values, attr);
    if (!href_begin) return NULL;

    char *link_begin = strchr(href_begin, '"');
    if (!link_begin) link_begin = strchr(href_begin, '\'');
    if (!link_begin) return NULL;
    else link_begin += 1;

    char *link_end = strchr(link_begin, '"');
    if (!link_end) link_end = strchr(link_begin, '\'');
    if (!link_end) return NULL;
    else link_end -= 1;

    int link_len = link_end - link_begin + 1;
    char *link = (char *)calloc(link_len + 1, sizeof (char));
    if (!link) {
        fprintf(stderr, "link calloc error\n");
        //free_opts();
        exit(1);
    }

    strncpy(link, link_begin, link_len);
    link[link_len] = '\0';
    return link;
}

void get_atag(mainpage_t *mainpage, FILE *mainpagep) {
    atag_t *atag = (atag_t *)calloc(1, sizeof (atag_t));
    if (!atag) {
        fprintf(stderr, "atag calloc error\n");
        exit(1);
    }
    atag->href = get_tagattr(mainpagep, "href");

    if (atag->href) {
        if (!mainpage->atags) {
            mainpage->atags = atag;
        } else {
            atag_t *atag_ite = mainpage->atags;
            while (atag_ite->next) {
                atag_ite = atag_ite->next;
            }
            atag_ite->next = atag;
        }
    } else {
        free(atag);
    }
}

void get_imgtag(mainpage_t *mainpage, FILE *mainpagep) {
    imgtag_t *imgtag = (imgtag_t *)calloc(1, sizeof (imgtag_t));
    if (!imgtag) {
        fprintf(stderr, "imgtag calloc error\n");
        exit(1);
    }
    imgtag->src = get_tagattr(mainpagep, "src");

    if (imgtag->src) {
        if (!mainpage->imgtags) {
            mainpage->imgtags = imgtag;
        } else {
            imgtag_t *imgtag_ite = mainpage->imgtags;
            while (imgtag_ite->next) {
                imgtag_ite = imgtag_ite->next;
            }
            imgtag_ite->next = imgtag;
        }
    } else {
        free(imgtag);
    }
}

void get_token(FILE *mainpagep, char *token, int token_size) {
    memset(token, 0, token_size);
    char c;
    for (int i = 0; i < token_size - 1; i++) {
        if ((c = fgetc(mainpagep)) == EOF) return;
        if (c == ' ' || c == '>') return;
        token[i] = c;
    }
}

mainpage_t * parse_mainpage(opts_t *opts) {
    FILE *mainpagep = fopen(opts->mainpage_name, "r");
    if (!mainpagep) {
        fprintf(stderr, "open mainpage failed.\n");
        //free_opts()
        exit(1);
    }

    mainpage_t *mainpage = (mainpage_t *)calloc(1, sizeof (mainpage_t));
    if (!mainpage) {
        fprintf(stderr, "mainpage calloc error\n");
        // free_opts();
        exit(1);
    }
    char c;
    int token_size = 10;
    char token[token_size];
    while ((c = fgetc(mainpagep)) != EOF) {
        if (c == '<') {
            get_token(mainpagep, token, token_size);
            if (strlen(token)) {
                if (!strcmp(token, "a")) {
                    get_atag(mainpage, mainpagep);
                    continue;
                } else if (!strcmp(token, "img")) {
                    get_imgtag(mainpage, mainpagep);
                    continue;
                }
            }
        }
    }
    fclose(mainpagep);
    return mainpage;
}

link_t * get_atag_links(opts_t *opts, mainpage_t *mainpage) {
    link_t *links = NULL;
    link_t *ite = NULL;
    atag_t *atag = mainpage->atags;
    while (atag) {
        if (strstr(atag->href, opts->targetfmt)) {
            link_t *link = (link_t *)calloc(1, sizeof (link_t));
            if (!link) {
                fprintf(stderr, "links calloc error\n");
                // free_opts()
                exit(1);
            }
            link->value = atag->href;
            link->next = NULL;
            if (!links) links = link;
            if (!ite) {
                ite = link;
            } else {
                ite->next = link;
                ite = link;
            }
        }
        atag = atag->next;
    }

    return links;
}

link_t * get_imgtag_links(opts_t *opts, mainpage_t *mainpage) {
    link_t *links = NULL;
    link_t *ite = NULL;
    imgtag_t *imgtag = mainpage->imgtags;
    while (imgtag) {
        if (strstr(imgtag->src, opts->targetfmt)) {
            link_t *link = (link_t *)calloc(1, sizeof (link_t));
            if (!link) {
                fprintf(stderr, "links calloc error\n");
                // free_opts()
                exit(1);
            }
            link->value = imgtag->src;
            link->next = NULL;
            if (!links) links = link;
            if (!ite) {
                ite = link;
            } else {
                ite->next = link;
                ite = link;
            }
        }
        imgtag = imgtag->next;
    }

    return links;
}

link_t * get_tag_links(opts_t *opts, mainpage_t *mainpage) {
    switch (opts->targettag) {
        case A_TAG:   return get_atag_links(opts, mainpage);
        case IMG_TAG: return get_imgtag_links(opts, mainpage);
    }
    return NULL;
}

void output_links(opts_t *opts, link_t *links) {
    FILE *linksp = fopen(opts->links_file, "w+");
    if (!linksp) {
        fprintf(stderr, "open linksp failed.\n");
        //free_opts()
        exit(1);
    }
    link_t *ite = links;
    while (ite) {
        fputs(ite->value, linksp);
        fputc('\n', linksp);
        ite = ite->next;
    }
    fclose(linksp);
}

void wget_confilm(link_t *links) {
    link_t *ite = links;
    int links_cnt = 0;
    printf("====confilm download files====\n");
    while (ite) {
        printf("%s\n", ite->value);
        ite = ite->next;
        links_cnt++;
    }
    printf("=============================\n");
    printf("Do you want to download %d files? (y/n) > ", links_cnt);
    char input;
    scanf("%c", &input);
    if (input == 'y') {
        printf("Start download.\n");
    } else {
        printf("Canceled.\n");
        exit(1);
    }
}

void wget_linksfile(opts_t *opts, link_t *links) {
    output_links(opts, links);
    wget_confilm(links);
    char wgetcmd[2048];
    if (opts->sequential_save == NULL) {
        sprintf(wgetcmd, "wget -a %s -i %s", opts->wget_logfile, opts->links_file);
        printf("[wget command] %s\n", wgetcmd);
        system(wgetcmd);
        return;
    }

    link_t *ite = links;
    int links_cnt = 0;
    while (ite) {
        sprintf(wgetcmd, "wget -a %s -O %03d.%s %s", opts->wget_logfile, links_cnt, opts->sequential_save, ite->value);
        printf("[wget command] %s\n", wgetcmd);
        system(wgetcmd);
        ite = ite->next;
        links_cnt++;
    }
}

int main(int argc, char **argv) {
    opts_t *opts = parse_opts(argc, argv);
    default_opts(opts);
    mk_savedir(opts);
    cd_savedir(opts);
    wget_mainpage(opts);
    mainpage_t *mainpage = parse_mainpage(opts);
    link_t *links = get_tag_links(opts, mainpage);
    wget_linksfile(opts, links);
}
