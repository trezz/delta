#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "delta/strmap.h"
#include "delta/vec.h"

/** Holds the inputs arguments. */
typedef struct args {
    int help;
    /* Date range. Pointer to the input argument or NULL if no range. */
    char* range;
    /* Number of results to display. Value 0 behave as if -q were given and
     * the total number of queries is displayed. */
    int num;

    /* Input file paths. */
    vec_t(char*) files;
} args_t;

/** Print usage */
static void usage() {
    printf("Usage: qex [-h] [-r RANGE] [-n NUM] FILE [FILE ...]\n");
}

/** Print help */
static void help() {
    usage();
    printf(
        "qex is a command line tool which takes as input TSV files containing\n"
        "any number of lines that follow this format\n"
        "`<YEAR>-<MONTH>-<DAY> <HOUR>:<MIN>:<SEC>\\t<QUERY>\\n` and options\n"
        "which parameters the outputs to extract from the input queries "
        "files.\n"
        "Depending on the input options, qex is able to output The number of\n"
        "distinct queries done during a specific time range. Follows some\n"
        "example use-cases.\n\n"
        "Options:\n"
        "  FILE      input TSV (tab separated values) files.\n"
        "  -h        Display help and exit.\n"
        "  -r RANGE  Optional parameter specifying the date range from which\n"
        "            queries are extracted.\n"
        "  -n NUM    If present, extract the NUM most popular queries done\n"
        "            within input files, optionally in the parametered date\n"
        "            range.\n");
}

/**
 * Parse input arguments, manage basic errors and returns a filled InputArgs
 * structure. It is the responsibility of the caller to print help and such,
 * this function automatically display usage and exit if necessary and exit
 * on error. */
static args_t parse_options(int argc, char** argv) {
    args_t args;

    args.help = 0;
    args.range = NULL;
    args.num = 0;
    args.files = NULL;

    if (argc == 1) {
        /* print usage and early exit */
        usage();
        exit(1);
    }

    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-h", argv[i])) {
            /* parse help option */
            args.help = 1;
        } else if (0 == strcmp("-r", argv[i])) {
            /* parse range option with a required argument */
            ++i;
            if (i == argc) {
                fprintf(stderr,
                        "error: -r option requires an argument. Use qex -h for "
                        "details\n");
                exit(1);
            } else {
                args.range = argv[i];
            }
        } else if (0 == strcmp("-n", argv[i])) {
            /* parse num option with a required argument */
            ++i;
            if (i == argc) {
                fprintf(stderr,
                        "error: -n option requires an argument. Use qex -h for "
                        "details\n");
                exit(1);
            } else {
                char* end = NULL;
                args.num = strtoll(argv[i], &end, 10);
                if (end == argv[i]) {
                    fprintf(
                        stderr,
                        "error: integer expected as argument of -n option\n");
                    exit(1);
                }
            }
        } else {
            /* assume to be an input file */
            {
                FILE* f = fopen(argv[i], "r");
                if (f == NULL) {
                    fprintf(stderr, "error: unknown input file %s\n", argv[i]);
                    exit(1);
                }
                fclose(f);
                if (args.files == NULL) {
                    args.files = vec_make(char*, 0, 1);
                }
                vec_append(&args.files, argv[i]);
            }
        }
    }

    return args;
}

typedef struct {
    int year, month, day;
    /* Second part of the timestamp */
    int hour, minute, second;
    char* end;
} range_t;

int toint(const char* s, char** next) {
    int result = 0;

    while (s && (*s == ' ' || *s == '\t')) ++s;

    for (; s && *s >= '0' && *s <= '9'; ++s) {
        result *= 10;
        result += (*s - '0');
    }

    if (next) {
        *next = (char*)s;
    }

    return result;
}

static void parse_range(range_t* r, const char* range) {
    /* fill a table of 6 dates with the string range. This loop accepts
     * wildcards characters (*) */
    int date[6] = {-1, -1, -1, -1, -1, -1};
    char* next_token = NULL;

    for (size_t i = 0; range && i < 6; ++i) {
        date[i] = toint(range, &next_token);
        if (range == next_token && *range == '*') {
            /* skipping wildcard character '*' + '-' */
            date[i] = -1;
            range++;
            if (*range != 0) range++;
        } else if (range == next_token) {
            /* invalid character found where integer conversion should occured.
             * reset current date value to -1 and stop conversion. */
            date[i] = -1;
            break;
        } else if (*next_token == 0) {
            break;
        } else {
            /* eventually step over separator character such as '-', ' ', '\t'
             * or ':' */
            range = next_token + 1;
        }
    }

    /* fill attributes */
    r->end = next_token;
    r->year = date[0];
    r->month = date[1];
    r->day = date[2];
    r->hour = date[3];
    r->minute = date[4];
    r->second = date[5];
}

typedef struct qex {
    range_t _user_range;
    /**< User-defined range given as constructor's input argument */
    range_t _range;
    /**< Current range object being updated at each line parsing */
    strmap_t(size_t) _queries_in_range;
    /**< Queries in requested range */
    strmap_t(vec_t(str_t)) _popular_queries;
} qex_t;

static void qex_init(qex_t* q, char* range) {
    q->_queries_in_range = strmap_make(size_t, 0);
    q->_popular_queries = strmap_make(str_t*, 0);
    parse_range(&q->_user_range, range);
}

static void qex_del(qex_t* q) {
    strmap_del(q->_queries_in_range);

    for (strmap_iterator_t it = strmap_iterator(q->_popular_queries);
         strmap_next(&it);) {
        vec_t(str_t)* queries_ptr = it.val_ptr;
        vec_del(*queries_ptr);
    }
    strmap_del(q->_popular_queries);
}

static int qex_is_equal(const range_t* range, const range_t* user_range) {
    return ((user_range->year & range->year) == range->year) &&
           ((user_range->month & range->month) == range->month) &&
           ((user_range->day & range->day) == range->day) &&
           ((user_range->hour & range->hour) == range->hour) &&
           ((user_range->minute & range->minute) == range->minute) &&
           ((user_range->second & range->second) == range->second);
}

static char* index_tsv_line(qex_t* q, char* line, size_t lineno) {
    parse_range(&q->_range, line);
    line = q->_range.end;

    /* next_token should point to the character preceding the query, which
     * is `\t` in a TSV file. If not then an invalid line was given as input
     * argument.
     * The query is pointed by line here.
     */
    if (*line != '\t') {
        return NULL;
    }
    char* query = ++line;

    /* go to end of query and save its length */
    while (*line != '\n' && *line != '\r' && *line != 0) {
        ++line;
    }
    size_t query_size = line - query;

    /* remove endline characters and step to next line */
    while (*line == '\n' || *line == '\r') {
        *line = 0;
        ++line;
    }

    /* here, the entire line is parsed. If the range does not match with the
     * requested user one, do nothing more. */
    if (qex_is_equal(&q->_range, &q->_user_range)) {
        query[query_size] = 0;
        str_t key = {.data = query, .len = query_size};
        size_t* maybe_n = strmap_at(q->_queries_in_range, key);
        if (maybe_n) {
            ++(*maybe_n);
        } else {
            strmap_add(&q->_queries_in_range, key, 1);
        }
    }

    /* returns null if this is the end of file */
    return *line != 0 ? line : 0;
}

static void build_most_popular_queries_set(qex_t* q) {
    for (strmap_iterator_t it = strmap_iterator(q->_queries_in_range);
         strmap_next(&it);) {
        const size_t n = *(int*)it.val_ptr;
        char buf[100];

        const size_t key_len = sprintf(buf, "%zu", n);
        const str_t key = {.data = buf, .len = key_len};

        vec_t(str_t)* maybe_queries = strmap_at(q->_popular_queries, key);
        if (maybe_queries) {
            vec_append(maybe_queries, it.key);
        } else {
            vec_t(str_t) queries = vec_make(str_t, 1, 1);
            queries[0] = it.key;
            strmap_add(&q->_popular_queries, key, queries);
        }
    }
}

static int popular_queries_sorter(vec_t(void) vec, size_t a, size_t b,
                                  void* ctx) {
    vec_t(str_t) v = vec;
    return atoi(v[a].data) > atoi(v[b].data);
}

static void print_nth_most_popular_queries(qex_t* q, size_t num) {
    vec_t(str_t) ns = vec_make(str_t, 0, strmap_len(q->_popular_queries));
    for (strmap_iterator_t it = strmap_iterator(q->_popular_queries);
         strmap_next(&it);) {
        vec_append(&ns, it.key);
    }
    vec_sort(ns, popular_queries_sorter, NULL);
    for (int i = 0; i < vec_len(ns); ++i) {
        str_t num_queries = ns[i];
        vec_t(str_t)* queries = strmap_at(q->_popular_queries, num_queries);
        for (int j = 0; j < vec_len(*queries); ++j) {
            str_t query = (*queries)[j];
            if (num-- == 0) {
                goto end;
            }
            printf("%s %s\n", query.data, num_queries.data);
        }
    }
end:
    vec_del(ns);
}

/************************* Entry point ***************************************/

int main(int argc, char** argv) {
    /* parse arguments */
    args_t args = parse_options(argc, argv);

    if (args.help) {
        /* show help and exit */
        help();
        exit(1);
    }

    /* index input files */
    qex_t qex;
    qex_init(&qex, args.range);
    strmap_t(char*) buffers = strmap_make(char*, vec_len(args.files));

    for (int i = 0; i < vec_len(args.files); ++i) {
        char* file = args.files[i];
        /* read whole file content into buffer */
        FILE* f = fopen(file, "r");
        fseek(f, 0, SEEK_END);
        size_t length = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* buf = malloc(sizeof(char) * length + 1);
        str_t key = {.data = file, .len = strlen(file)};
        strmap_add(&buffers, key, buf);
        if (fread(buf, length, 1, f) != 1) {
            printf("failed to read file\n");
            exit(1);
        }
        buf[length] = 0;
        fclose(f);

        /* index the file using Qex object and catch eventual errors */
        size_t lineno = 1;
        for (char* line = buf; line;
             line = index_tsv_line(&qex, line, lineno)) {
            if (line == NULL) {
                fprintf(stderr, "error\n");
                break;
            }
            ++lineno;
        }
    }

    /* extract queries based on input arguments */
    if (args.num == 0) {
        printf("%zu\n", strmap_len(qex._queries_in_range));
    } else {
        build_most_popular_queries_set(&qex);
        print_nth_most_popular_queries(&qex, args.num);
    }

    for (strmap_iterator_t it = strmap_iterator(buffers); strmap_next(&it);) {
        char** buffer = it.val_ptr;
        free(*buffer);
    }
    strmap_del(buffers);
    qex_del(&qex);

    vec_del(args.files);

    return 0;
}
