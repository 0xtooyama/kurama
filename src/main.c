#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

// tags
#define A_TAG   1
#define IMG_TAG 2

// attrs
#define HREF_ATTR 1
#define SRC_ATTR  2

typedef struct {
    char *url;
    int urllen;
    char *mainpage_name;
    char *savedir;
    char *targetfmt;
    char *wget_logfile;
    char *links_file;
    int targettag;
    int sequeantial_save;
} args_t;

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

args_t * parse_args(int argc, char **argv) {
    args_t *args = (args_t *)calloc(1, sizeof(args_t));
    if (!args) {
        fprintf(stderr, "args calloc error\n");
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        char *word = argv[i];
        if (word[0] == '-') {
            char opt[16];
            strcpy(opt, &word[1]);
            if (!strcmp(opt, "url")) {
                args->url = (char *)calloc(strlen(argv[i + 1]) + 1, sizeof(char));
                if (!args->url) {
                    fprintf(stderr, "args->url calloc error\n");
                    //free_args();
                    exit(1);
                }
                strcpy(args->url, argv[i + 1]);
                args->urllen = strlen(args->url);
                i++;
                continue;
            } else if (!strcmp(opt, "savedir")) {
                args->savedir = (char *)calloc(strlen(argv[i + 1]) + 1, sizeof(char));
                if (!args->savedir) {
                    fprintf(stderr, "args->savedir calloc error\n");
                    //free_args();
                    exit(1);
                }
                strcpy(args->savedir, argv[i + 1]);
                i++;
                continue;
            } else if (!strcmp(opt, "targetfmt")) {
                args->targetfmt = (char *)calloc(strlen(argv[i + 1]) + 1, sizeof(char));
                if (!args->targetfmt) {
                    fprintf(stderr, "args->targetfmt calloc error\n");
                    //free_args();
                    exit(1);
                }
                strcpy(args->targetfmt, argv[i + 1]);
                i++;
                continue;
            } else if (!strcmp(opt, "mainpage_name")) {
                args->mainpage_name = (char *)calloc(strlen(argv[i + 1]) + 1, sizeof(char));
                if (!args->mainpage_name) {
                    fprintf(stderr, "args->targetfmt calloc error\n");
                    //free_args();
                    exit(1);
                }
                strcpy(args->mainpage_name, argv[i + 1]);
                i++;
                continue;
            } else if (!strcmp(opt, "wget_logfile")) {
                args->wget_logfile = (char *)calloc(strlen(argv[i + 1]) + 1, sizeof(char));
                if (!args->wget_logfile) {
                    fprintf(stderr, "args->wget_logfile calloc error\n");
                    //free_args();
                    exit(1);
                }
                strcpy(args->wget_logfile, argv[i + 1]);
                i++;
                continue;
            } else if (!strcmp(opt, "targettag")) {
                char *targettag = (char *)calloc(strlen(argv[i + 1]) + 1, sizeof(char));
                if (!targettag) {
                    fprintf(stderr, "targettag calloc error\n");
                    //free_args();
                    exit(1);
                }
                strcpy(targettag, argv[i + 1]);
                if (!strcmp(targettag, "a")) {
                    args->targettag = A_TAG;
                } else if (!strcmp(targettag, "img")) {
                    args->targettag = IMG_TAG;
                }
                i++;
                continue;
            } else if (!strcmp(opt, "sequentialsave")) {
                args->sequeantial_save = 1;
                continue;
            } else {
                fprintf(stderr, "option '%s' is not decrared\n", opt);
                //free_args();
                exit(1);
            }
        } else {
            fprintf(stderr, "need options to execute\n");
            //free_args();
            exit(1);
        }
    }
    return args;
}

char * get_now() {
    time_t t = time(NULL);
    if (t == -1) {
        fprintf(stderr, "get time erorr\n");
        // free_args()
        exit(1);
    }
    struct tm *tm = localtime(&t);
    if (tm == NULL) {
        fprintf(stderr, "get localtime erorr\n");
        // free_args()
        exit(1);
    }
    char *now = (char *)calloc(strlen("yyyyMMddhhmmss") + 1, sizeof (char));
    if (!now) {
        fprintf(stderr, "now calloc error\n");
        //free_args();
        exit(1);
    }
    sprintf(now, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday + 1, tm->tm_hour, tm->tm_min, tm->tm_sec);
    return now;
}

void default_args(args_t *args) {
    if (args->savedir == NULL) {
        args->savedir = get_now();
    }

    if (args->mainpage_name == NULL) {
        char *mainpage_name = "main.html";
        args->mainpage_name = (char *)calloc(strlen(mainpage_name) + 1, sizeof (char));
        if (!args->mainpage_name) {
            fprintf(stderr, "args->mainpage_name calloc error\n");
            //free_args();
            exit(1);
        }
        strcpy(args->mainpage_name, mainpage_name);
    }

    if (args->wget_logfile == NULL) {
        char *wget_logfile = "wget_log.txt";
        args->wget_logfile = (char *)calloc(strlen(wget_logfile) + 1, sizeof (char));
        if (!args->wget_logfile) {
            fprintf(stderr, "args->wget_logfile calloc error\n");
            //free_args();
            exit(1);
        }
        strcpy(args->wget_logfile, wget_logfile);
    }

    if (args->links_file == NULL) {
        char *links_file = "links.txt";
        args->links_file = (char *)calloc(strlen(links_file) + 1, sizeof (char));
        if (!args->links_file) {
            fprintf(stderr, "args->links_file calloc error\n");
            //free_args();
            exit(1);
        }
        strcpy(args->links_file, links_file);
    }

    if (args->targettag == 0) args->targettag = A_TAG;
    if (args->sequeantial_save == 0) args->sequeantial_save = 0;
}

void mk_savedir(args_t *args) {
    char *savedir = args->savedir;
    DIR *savedirp = opendir(savedir);
    if (savedirp) {
        fprintf(stderr, "directory %s is existed.\n", savedir);
        closedir(savedirp);
        //free_args();
        exit(1);
    }

    int rc = mkdir(savedir, 0755);
    if (rc == -1) {
        fprintf(stderr, "mkdir %s error\n", savedir);
        exit(1);
    }
}

void cd_savedir(args_t *args) {
    int rc = chdir(args->savedir);
    if (rc == -1) {
        fprintf(stderr, "chdir %s failed.\n", args->savedir);
        //free_args()
        exit(1);
    }
}

void wget_mainpage(args_t *args) {
    char wgetcmd[2048];
    sprintf(wgetcmd, "wget -o %s -O %s %s", args->wget_logfile, args->mainpage_name, args->url);
    printf("[wget command] %s\n", wgetcmd);
    system(wgetcmd);
    FILE *mainpagep = fopen(args->mainpage_name, "r");
    if (!mainpagep) {
        fprintf(stderr, "wget mainpage failed.\n");
        //free_args()
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
        //free_args();
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

mainpage_t * parse_mainpage(args_t *args) {
    FILE *mainpagep = fopen(args->mainpage_name, "r");
    if (!mainpagep) {
        fprintf(stderr, "open mainpage failed.\n");
        //free_args()
        exit(1);
    }

    mainpage_t *mainpage = (mainpage_t *)calloc(1, sizeof (mainpage_t));
    if (!mainpage) {
        fprintf(stderr, "mainpage calloc error\n");
        // free_args();
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

link_t * get_atag_links(args_t *args, mainpage_t *mainpage) {
    link_t *links = NULL;
    link_t *ite = NULL;
    atag_t *atag = mainpage->atags;
    while (atag) {
        if (strstr(atag->href, args->targetfmt)) {
            link_t *link = (link_t *)calloc(1, sizeof (link_t));
            if (!link) {
                fprintf(stderr, "links calloc error\n");
                // free_args()
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

link_t * get_imgtag_links(args_t *args, mainpage_t *mainpage) {
    link_t *links = NULL;
    link_t *ite = NULL;
    imgtag_t *imgtag = mainpage->imgtags;
    while (imgtag) {
        if (strstr(imgtag->src, args->targetfmt)) {
            link_t *link = (link_t *)calloc(1, sizeof (link_t));
            if (!link) {
                fprintf(stderr, "links calloc error\n");
                // free_args()
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

link_t * get_tag_links(args_t *args, mainpage_t *mainpage) {
    switch (args->targettag) {
        case A_TAG:   return get_atag_links(args, mainpage);
        case IMG_TAG: return get_imgtag_links(args, mainpage);
    }
    return NULL;
}

void output_links(args_t *args, link_t *links) {
    FILE *linksp = fopen(args->links_file, "w+");
    if (!linksp) {
        fprintf(stderr, "open linksp failed.\n");
        //free_args()
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

void wget_linksfile(args_t *args, link_t *links) {
    output_links(args, links);
    wget_confilm(links);
    char wgetcmd[2048];
    if (!args->sequeantial_save) {
        sprintf(wgetcmd, "wget -a %s -i %s", args->wget_logfile, args->links_file);
        printf("[wget command] %s\n", wgetcmd);
        system(wgetcmd);
        return;
    }

    link_t *ite = links;
    int links_cnt = 0;
    while (ite) {
        sprintf(wgetcmd, "wget -a %s -O %03d %s", args->wget_logfile, links_cnt, ite->value);
        printf("[wget command] %s\n", wgetcmd);
        system(wgetcmd);
        ite = ite->next;
        links_cnt++;
    }
}

int main(int argc, char **argv) {
    args_t *args = parse_args(argc, argv);
    default_args(args);
    mk_savedir(args);
    cd_savedir(args);
    wget_mainpage(args);
    mainpage_t *mainpage = parse_mainpage(args);
    link_t *links = get_tag_links(args, mainpage);
    wget_linksfile(args, links);
}
