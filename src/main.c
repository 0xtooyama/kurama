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
    {"savedir", required_argument, NULL, (int)'s'},
    {"format", required_argument, NULL, (int)'f'},
    {"logfile", required_argument, NULL, (int)'l'},
    {"tag", required_argument, NULL, (int)'t'},
    {"seqname", required_argument, NULL, (int)'1'},
    {"redownload", no_argument, NULL, (int)'r'},
    {"skipconf", no_argument, NULL, (int)'2'},
    {0, 0, 0, 0}
};

typedef struct {
    char *savedir;
    char *targetfmt;
    char *wget_logfile;
    int targettag;
    char *sequential_save;
    int redownload;
    int skipconf;

    char *url;
    int urllen;
    char *mainpage_name;
    char *links_file;
    char *url_scheme;
    char *url_root;
    int *redownload_seq_name;
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

typedef struct list_t {
    char *value;
    struct list_t *next;
} list_t;

void show_help() {
    printf("Usage: kurama [OPTION]... -f FORMAT URL\n");
    printf("   or: kurama [OPTION]... -r\n");
    printf("Download the link contents included in the page specified by URL using wget.\n");
    printf("Only the link contents containing the string specified by FORMAT in the URL \n");
    printf("will be downloaded.\n");
    printf("\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");
    printf("  -h, --help               display this help and exit.\n");
    printf("  -f, --format FORMAT      The FORMAT is a string contained in the link to \n");
    printf("                           be downloaded.\n");
    printf("  -s, --savedir DIRNAME    The contents downloaded by wget is stored to \n");
    printf("                           the directory specified by DIRNAME.\n");
    printf("  -l, --logfile LOGFILE    specify the logfile.\n");
    printf("  -t, --tag TAG            specify the html tag containing the url of content \n");
    printf("                           to be downloaded.\n");
    printf("  -r, --redownload         redownload the failed file.\n");
    printf("      --seqname SUFFIX     rename the downloaded files name to 0-based sequential \n");
    printf("                           number and attached SUFFIX.\n");
    exit(0);
}

opts_t * parse_opts(int argc, char **argv) {
    opts_t *opts = (opts_t *)calloc(1, sizeof(opts_t));
    if (!opts) {
        fprintf(stderr, "opts calloc error\n");
        exit(1);
    }
    //TODO init_opts_t(opts);

    int opt = 0;
    while ((opt = getopt_long(argc, argv, "s:f:l:t:rh", long_options, NULL)) != -1) {
        char *opt_value = NULL;
        if (optarg != 0) {
            opt_value = (char *)calloc(strlen(optarg) + 1, sizeof(char));
            if (!opt_value) {
                fprintf(stderr, "opt_value calloc error\n");
                //free_opts();
                exit(1);
            }
        }

        if (optarg == NULL) free(opt_value);
        else strcpy(opt_value, optarg);

        switch (opt) {
            case 's':
                opts->savedir = opt_value;
                break;
            case 'f':
                opts->targetfmt = opt_value;
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
            case 'r':
                opts->redownload = 1;
                break;
            case 'h': 
                show_help();
                break;
            case '2':
                opts->skipconf = 1;
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

    //TODO check consistency

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

char * get_url_scheme(char *url) {
    char *c = strchr(url, ':') + 1;

    char *url_scheme = (char *)calloc(c - url + 1, sizeof (char));
    if (!url_scheme) {
        fprintf(stderr, "url_scheme calloc error\n");
        exit(1);
    }
    strncpy(url_scheme, url, c - url);
    return url_scheme;
}

char * get_url_root(char *url) {
    char *c = strchr(url, '/');
    c = strchr(c + 1, '/');
    c = strchr(c + 1, '/');

    char *url_root = (char *)calloc(c - url + 1, sizeof (char));
    if (!url_root) {
        fprintf(stderr, "url_root calloc error\n");
        exit(1);
    }
    strncpy(url_root, url, c - url);
    return url_root;
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

    if (opts->url) {
        opts->url_scheme = get_url_scheme(opts->url);
        opts->url_root = get_url_root(opts->url);
    }
}

void check_opts(opts_t *opts) {
    if (opts->redownload) {
        if (opts->targetfmt || opts->targettag || opts->url) {
            fprintf(stderr, "-r, --redownload can not be used with -f, -t and URL.\n");
            // free_opts()
            exit(1);
        }
        return;
    }

    if (!opts->url) {
        fprintf(stderr, "URL is required.\n");
        // free_opts()
        exit(1);
    }

    if (!opts->targetfmt) {
        fprintf(stderr, "-f is required.\n");
        // free_opts()
        exit(1);
    }
}

opts_t *init_opts(int argc, char **argv) {
    opts_t *opts = parse_opts(argc, argv);
    check_opts(opts);
    default_opts(opts);
    return opts;
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

char * get_url(char *url_scheme, char *link) {
    char *url = NULL;
    if (link[0] != '/' && link[1] != '/') {
        url = (char *)calloc(strlen(link) + 1, sizeof (char));
        if (!url) {
            fprintf(stderr, "url calloc error\n");
            exit(1);
        }
        strcpy(url, link);
        return url;
    }
    url = (char *)calloc(strlen(url_scheme) + strlen(link) + 1, sizeof (char));
    if (!url) {
        fprintf(stderr, "url calloc error\n");
        exit(1);
    }
    strcpy(url, url_scheme);
    strcpy(&url[strlen(url_scheme)], link);
    return url;
}

list_t * get_atag_links(opts_t *opts, mainpage_t *mainpage) {
    list_t *links = NULL;
    list_t *ite = NULL;
    atag_t *atag = mainpage->atags;
    while (atag) {
        if (strstr(atag->href, opts->targetfmt)) {
            list_t *link = (list_t *)calloc(1, sizeof (list_t));
            if (!link) {
                fprintf(stderr, "links calloc error\n");
                // free_opts()
                exit(1);
            }
            link->value = get_url(opts->url_scheme, atag->href);
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

list_t * get_imgtag_links(opts_t *opts, mainpage_t *mainpage) {
    list_t *links = NULL;
    list_t *ite = NULL;
    imgtag_t *imgtag = mainpage->imgtags;
    while (imgtag) {
        if (strstr(imgtag->src, opts->targetfmt)) {
            list_t *link = (list_t *)calloc(1, sizeof (list_t));
            if (!link) {
                fprintf(stderr, "links calloc error\n");
                // free_opts()
                exit(1);
            }
            link->value = get_url(opts->url_scheme, imgtag->src);
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

list_t * get_tag_links(opts_t *opts, mainpage_t *mainpage) {
    switch (opts->targettag) {
        case A_TAG:   return get_atag_links(opts, mainpage);
        case IMG_TAG: return get_imgtag_links(opts, mainpage);
    }
    return NULL;
}

void output_links(opts_t *opts, list_t *links) {
    FILE *linksp = fopen(opts->links_file, "w+");
    if (!linksp) {
        fprintf(stderr, "open linksp failed.\n");
        //free_opts()
        exit(1);
    }
    list_t *ite = links;
    while (ite) {
        fputs(ite->value, linksp);
        fputc('\n', linksp);
        ite = ite->next;
    }
    fclose(linksp);
}

void wget_confilm(list_t *links, int skipconf) {
    list_t *ite = links;
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
    if (skipconf) input = 'y';
    else scanf("%c", &input);
    if (input == 'y') {
        printf("Start download.\n");
    } else {
        printf("Canceled.\n");
        exit(1);
    }
}

void wget_linksfile(opts_t *opts, list_t *links) {
    output_links(opts, links);
    wget_confilm(links, opts->skipconf);
    char wgetcmd[2048];
    if (opts->sequential_save == NULL) {
        sprintf(wgetcmd, "wget -a %s -i %s", opts->wget_logfile, opts->links_file);
        printf("[wget command] %s\n", wgetcmd);
        system(wgetcmd);
        return;
    }

    list_t *ite = links;
    int links_cnt = 0;
    while (ite) {
        sprintf(wgetcmd, "wget -a %s -O %03d.%s %s", opts->wget_logfile, links_cnt, opts->sequential_save, ite->value);
        printf("[wget command] %s\n", wgetcmd);
        system(wgetcmd);
        ite = ite->next;
        links_cnt++;
    }
}

void wget_redownload(opts_t *opts, list_t *links) {
    wget_confilm(links, opts->skipconf);
    char wgetcmd[2048];
    list_t *ite = links;

    if (opts->sequential_save == NULL) {
        while (ite) {
            sprintf(wgetcmd, "wget -a %s %s", opts->wget_logfile, ite->value);
            printf("[wget command] %s\n", wgetcmd);
            system(wgetcmd);
            ite = ite->next;
        }
        return;
    }

    int i = 0;
    while (ite) {
        sprintf(wgetcmd, "wget -a %s -O %03d.%s %s", opts->wget_logfile, opts->redownload_seq_name[i], opts->sequential_save, ite->value);
        printf("[wget command] %s\n", wgetcmd);
        system(wgetcmd);
        ite = ite->next;
        i++;
    }
}

void show_result(opts_t *opts) {
    FILE *wget_logfile = fopen(opts->wget_logfile, "r");
    if (!wget_logfile) {
        fprintf(stderr, "wget_logfile open failed.\n");
        //free_opts()
        exit(1);
    }
    char result[1024];
    while (fgets(result, 1024, wget_logfile) != NULL) {}
    fclose(wget_logfile);
    printf("Finish.\n[Result] %s\n", result);
}

int get_file_max_row(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "file '%s' fopen error.\n", filename);
        exit(1);
    }
    int row_cnt = 0;
    char c;
    while ((c = fgetc(file)) != EOF) 
        if (c == '\n') row_cnt++;
    fclose(file);
    return row_cnt;
}

list_t * get_redownload_links_seq(opts_t *opts, struct dirent **entries, int entry_max) {
    int links_file_max_row = get_file_max_row(opts->links_file);
    opts->redownload_seq_name = (int *)calloc(links_file_max_row, sizeof (int));
    if (!opts->redownload_seq_name) {
        fprintf(stderr, "redonwload_seq_name calloc error.\n");
        //free_opts();
        exit(1);
    }

    char *seq_name = (char *)calloc(3 + strlen(opts->sequential_save) + 1, 1);
    if (!seq_name) {
        fprintf(stderr, "seq_name calloc error.\n");
        exit(1);
    }
    int seq_name_cnt = 0;
    int entry_cnt = 0;
    int redownload_seq_name_cnt = 0;
    for (int seq_name_cnt = 0; seq_name_cnt < links_file_max_row; seq_name_cnt++) {
        sprintf(seq_name, "%03d.%s", seq_name_cnt, opts->sequential_save); 

        if (seq_name_cnt >= (entry_max - 1)) {
            opts->redownload_seq_name[redownload_seq_name_cnt] = seq_name_cnt;
            redownload_seq_name_cnt++;
            continue;
        }

        while (seq_name_cnt < entry_max) {
            if ((entries[entry_cnt]->d_name)[0] == '.') {
                entry_cnt++;
                continue;
            }
            if (strstr(entries[entry_cnt]->d_name, opts->sequential_save)) {
                if (strcmp(entries[entry_cnt]->d_name, seq_name) == 0) {
                    entry_cnt++;
                    break;
                }
                opts->redownload_seq_name[redownload_seq_name_cnt] = seq_name_cnt;
                redownload_seq_name_cnt++;
                break;
            }
            entry_cnt++;
        }
    }

    FILE *links_file = fopen(opts->links_file, "r");
    if (!links_file) {
        fprintf(stderr, "file '%s' fopen error.\n", opts->links_file);
        exit(1);
    }
    list_t *head = NULL;
    list_t *pre = NULL;
    char c;
    redownload_seq_name_cnt = 0;
    for (int i = 0; i < links_file_max_row; i++) {
        if (i == opts->redownload_seq_name[redownload_seq_name_cnt]) {
            redownload_seq_name_cnt++;
            list_t *list = (list_t *)calloc(1, sizeof (list_t));
            if (!list) {
                fprintf(stderr, "list calloc error.\n");
                exit(1);
            }
            list->next = NULL;
            int char_cnt = 0;
            while ((c = fgetc(links_file)) != EOF) {
                char_cnt++;
                if (c == '\n') break;
            }
            fseek(links_file, -char_cnt, SEEK_CUR);
            list->value = (char *)calloc(char_cnt + 1, sizeof (char));
            if (!list->value) {
                fprintf(stderr, "list->value calloc error.\n");
                exit(1);
            }
            fgets(list->value, char_cnt, links_file);

            if (!head) head = pre = list;
            else {
                pre->next = list;
                pre = list;
            }
        }
        while ((c = fgetc(links_file)) != EOF) {
            if (c == '\n') break;
        }
    }
    fclose(links_file);
    return head;
}

list_t * get_redownload_links(opts_t *opts) {
    DIR *save_dir = opendir(opts->savedir);
    if (!save_dir) {
        fprintf(stderr, "open directory '%s' error.\n", opts->savedir);
        //free_opts();
        exit(1);
    }
    closedir(save_dir);
    struct dirent **entries;
    int entry_cnt = scandir(opts->savedir, &entries, NULL, alphasort);
    if (entry_cnt == -1) {
        fprintf(stderr, "directory '%s' is empty.\n", opts->savedir);
        //free_opts();
        exit(1);
    }

    if (opts->sequential_save) {
        return get_redownload_links_seq(opts, entries, entry_cnt);
    }

    int links_file_max_row = get_file_max_row(opts->links_file);
    FILE *links_file = fopen(opts->links_file, "r");
    if (!links_file) {
        fprintf(stderr, "file '%s' fopen error.\n", opts->links_file);
        exit(1);
    }
    list_t *head = NULL;
    list_t *pre = NULL;
    char c;
    char *link_name = NULL;
    int link_name_len = 0;
    int link_len = 0;
    int slash_index = 0;
    for (int i = 0; i < links_file_max_row; i++) {
        if (link_name) free(link_name);
        link_len = 0;
        while ((c = fgetc(links_file)) != EOF) {
            link_len++;
            if (c == '/') {
                slash_index = link_len;
            }
            if (c == '\n') break;
        }
        link_name_len = link_len - slash_index;
        fseek(links_file, -link_name_len, SEEK_CUR);
        link_name = (char *)calloc(link_name_len + 1, sizeof (char));
        if (!link_name) {
            fprintf(stderr, "link_name calloc error.\n");
            exit(1);
        }
        fgets(link_name, link_name_len, links_file);

        int exist_flag = 0;
        for (int j = 0; j < entry_cnt; j++) {
            if (strcmp(link_name, entries[j]->d_name) == 0) {
                exist_flag = 1;
                break;
            }
        }
        if (exist_flag) {
            fseek(links_file, 1, SEEK_CUR);
            continue;
        }

        fseek(links_file, -(link_len - 1), SEEK_CUR);
        list_t *list = (list_t *)calloc(1, sizeof (list_t));
        if (!list) {
            fprintf(stderr, "list calloc error.\n");
            exit(1);
        }
        list->next = NULL;
        list->value = (char *)calloc(link_len + 1, sizeof (char));
        if (!list->value) {
            fprintf(stderr, "list->value calloc error.\n");
            exit(1);
        }
        fgets(list->value, link_len, links_file);
        if (!head) head = pre = list;
        else {
            pre->next = list;
            pre = list;
        }
        fseek(links_file, 1, SEEK_CUR);
    }
    fclose(links_file);
    return head;
}

int main(int argc, char **argv) {
    opts_t *opts = init_opts(argc, argv);
    if (opts->redownload) {
        list_t *links = get_redownload_links(opts);
        wget_redownload(opts, links);
    } else {
        mk_savedir(opts);
        cd_savedir(opts);
        wget_mainpage(opts);
        mainpage_t *mainpage = parse_mainpage(opts);
        list_t *links = get_tag_links(opts, mainpage);
        wget_linksfile(opts, links);
        show_result(opts);
    }
}
