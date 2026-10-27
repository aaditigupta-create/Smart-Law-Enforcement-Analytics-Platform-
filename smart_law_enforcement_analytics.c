/*
 * Smart Law Enforcement Analytics Platform
 * DSA-II academic prototype using fictional incident records only.
 *
 * Algorithms demonstrated:
 * 1. Hash table (separate chaining) for case-ID lookup
 * 2. Merge sort for sorting by date or severity
 * 3. Breadth-First Search (BFS) for exploring linked cases
 * 4. Depth-First Search (DFS) for connected components
 * 5. Longest Common Subsequence (LCS) for keyword-sequence comparison
 *
 * Compile: gcc -std=c11 -Wall -Wextra -O2 smart_law_enforcement_analytics.c -o dsa_platform
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CASES 100
#define MAX_KEYWORDS 5
#define MAX_TEXT 40
#define HASH_SIZE 211

typedef struct {
    char id[12];
    char type[24];
    char area[24];
    char date[11];       /* YYYY-MM-DD */
    int severity;        /* 1 = Low, 2 = Medium, 3 = High */
    char keywords[MAX_KEYWORDS][MAX_TEXT];
    int keyword_count;
} CaseRecord;

typedef struct HashNode {
    char key[12];
    int record_index;
    struct HashNode *next;
} HashNode;

typedef struct {
    int to;
    int next;
} Edge;

static CaseRecord cases[MAX_CASES];
static int case_count = 0;
static HashNode *hash_table[HASH_SIZE];
static int head[MAX_CASES];
static Edge edges[MAX_CASES * MAX_CASES];
static int edge_count = 0;

static unsigned long hash_id(const char *s) {
    unsigned long h = 5381;
    while (*s) h = ((h << 5) + h) + (unsigned char)*s++;
    return h % HASH_SIZE;
}

static int add_case(const CaseRecord *record) {
    if (case_count >= MAX_CASES) return 0;
    cases[case_count] = *record;
    case_count++;
    return 1;
}

static void build_hash_index(void) {
    int i;
    for (i = 0; i < case_count; i++) {
        unsigned long bucket = hash_id(cases[i].id);
        HashNode *node = (HashNode *)malloc(sizeof(HashNode));
        if (!node) {
            fprintf(stderr, "Memory allocation failed while building hash index.\n");
            exit(EXIT_FAILURE);
        }
        strcpy(node->key, cases[i].id);
        node->record_index = i;
        node->next = hash_table[bucket];
        hash_table[bucket] = node;
    }
}

static int search_case_id(const char *id) {
    HashNode *node = hash_table[hash_id(id)];
    while (node) {
        if (strcmp(node->key, id) == 0) return node->record_index;
        node = node->next;
    }
    return -1;
}

static int severity_rank(const char *s) {
    if (strcmp(s, "High") == 0) return 3;
    if (strcmp(s, "Medium") == 0) return 2;
    return 1;
}

static void merge(CaseRecord a[], int left, int mid, int right, int by_severity) {
    int n1 = mid - left + 1, n2 = right - mid, i, j, k;
    CaseRecord *L = (CaseRecord *)malloc((size_t)n1 * sizeof(CaseRecord));
    CaseRecord *R = (CaseRecord *)malloc((size_t)n2 * sizeof(CaseRecord));
    if (!L || !R) {
        fprintf(stderr, "Memory allocation failed during merge sort.\n");
        free(L); free(R); exit(EXIT_FAILURE);
    }
    for (i = 0; i < n1; i++) L[i] = a[left + i];
    for (j = 0; j < n2; j++) R[j] = a[mid + 1 + j];
    i = 0; j = 0; k = left;
    while (i < n1 && j < n2) {
        int take_left;
        if (by_severity) {
            take_left = severity_rank(L[i].severity ? 
                (L[i].severity == 3 ? "High" : L[i].severity == 2 ? "Medium" : "Low") : "Low")
                >= severity_rank(R[j].severity ?
                (R[j].severity == 3 ? "High" : R[j].severity == 2 ? "Medium" : "Low") : "Low");
        } else {
            take_left = strcmp(L[i].date, R[j].date) <= 0;
        }
        if (take_left) a[k++] = L[i++];
        else a[k++] = R[j++];
    }
    while (i < n1) a[k++] = L[i++];
    while (j < n2) a[k++] = R[j++];
    free(L); free(R);
}

static void merge_sort(CaseRecord a[], int left, int right, int by_severity) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    merge_sort(a, left, mid, by_severity);
    merge_sort(a, mid + 1, right, by_severity);
    merge(a, left, mid, right, by_severity);
}

static void add_edge(int from, int to) {
    if (edge_count >= (int)(sizeof(edges) / sizeof(edges[0]))) return;
    edges[edge_count].to = to;
    edges[edge_count].next = head[from];
    head[from] = edge_count++;
}

static void build_graph(void) {
    int i, j;
    for (i = 0; i < case_count; i++) head[i] = -1;
    edge_count = 0;
    for (i = 0; i < case_count; i++) {
        for (j = i + 1; j < case_count; j++) {
            /* Link cases sharing crime type OR area; this is only a sample rule. */
            if (strcmp(cases[i].type, cases[j].type) == 0 ||
                strcmp(cases[i].area, cases[j].area) == 0) {
                add_edge(i, j);
                add_edge(j, i);
            }
        }
    }
}

static void bfs(int start) {
    int queue[MAX_CASES], front = 0, rear = 0;
    int visited[MAX_CASES] = {0};
    queue[rear++] = start;
    visited[start] = 1;
    printf("BFS from %s: ", cases[start].id);
    while (front < rear) {
        int u = queue[front++], e;
        printf("%s ", cases[u].id);
        for (e = head[u]; e != -1; e = edges[e].next) {
            int v = edges[e].to;
            if (!visited[v]) {
                visited[v] = 1;
                queue[rear++] = v;
            }
        }
    }
    printf("\n");
}

static void dfs_visit(int u, int visited[]) {
    int e;
    visited[u] = 1;
    printf("%s ", cases[u].id);
    for (e = head[u]; e != -1; e = edges[e].next) {
        int v = edges[e].to;
        if (!visited[v]) dfs_visit(v, visited);
    }
}

static void dfs_components(void) {
    int visited[MAX_CASES] = {0}, i, component = 0;
    printf("DFS connected groups:\n");
    for (i = 0; i < case_count; i++) {
        if (!visited[i]) {
            printf("Group %d: ", ++component);
            dfs_visit(i, visited);
            printf("\n");
        }
    }
}

static int lcs_length(const char *a[], int n, const char *b[], int m) {
    int dp[MAX_KEYWORDS + 1][MAX_KEYWORDS + 1] = {{0}};
    int i, j;
    for (i = 1; i <= n; i++) {
        for (j = 1; j <= m; j++) {
            if (strcmp(a[i - 1], b[j - 1]) == 0)
                dp[i][j] = dp[i - 1][j - 1] + 1;
            else
                dp[i][j] = dp[i - 1][j] > dp[i][j - 1] ?
                           dp[i - 1][j] : dp[i][j - 1];
        }
    }
    return dp[n][m];
}

static void print_case(const CaseRecord *c) {
    const char *sev = c->severity == 3 ? "High" : c->severity == 2 ? "Medium" : "Low";
    printf("%s | %s | %s | %s | %s | keywords: ",
           c->id, c->type, c->area, c->date, sev);
    for (int i = 0; i < c->keyword_count; i++)
        printf("%s%s", i ? ", " : "", c->keywords[i]);
    printf("\n");
}

static void free_hash_index(void) {
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode *node = hash_table[i];
        while (node) {
            HashNode *next = node->next;
            free(node);
            node = next;
        }
    }
}

int main(void) {
    CaseRecord sample[] = {
        {"C101","Theft","Zone A","2026-08-03",2,{"mobile","market"},2},
        {"C102","Theft","Zone A","2026-08-09",1,{"bike","parking"},2},
        {"C103","Burglary","Zone A","2026-08-14",3,{"night","lock broken"},2},
        {"C104","Burglary","Zone B","2026-08-18",3,{"night","window broken"},2},
        {"C105","Fraud","Zone C","2026-08-20",3,{"online","card"},2},
        {"C106","Fraud","Zone C","2026-08-23",2,{"online","upi"},2},
        {"C107","Assault","Zone D","2026-08-27",3,{"street","night"},2},
        {"C108","Assault","Zone D","2026-09-01",2,{"street","quarrel"},2}
    };
    const char *seq_a[] = {"night", "lock", "broken"};
    const char *seq_b[] = {"night", "window", "broken"};
    CaseRecord sorted[MAX_CASES];
    int found;

    for (size_t i = 0; i < sizeof(sample) / sizeof(sample[0]); i++)
        add_case(&sample[i]);

    build_hash_index();

    puts("=== Smart Law Enforcement Analytics Platform (Fictional Sample Data) ===");
    found = search_case_id("C105");
    if (found >= 0) {
        printf("\nHash lookup for C105:\n");
        print_case(&cases[found]);
    } else {
        puts("Case C105 not found.");
    }

    memcpy(sorted, cases, (size_t)case_count * sizeof(CaseRecord));
    merge_sort(sorted, 0, case_count - 1, 0);
    puts("\nRecords sorted by date:");
    for (int i = 0; i < case_count; i++) print_case(&sorted[i]);

    memcpy(sorted, cases, (size_t)case_count * sizeof(CaseRecord));
    merge_sort(sorted, 0, case_count - 1, 1);
    puts("\nRecords sorted by severity (High to Low):");
    for (int i = 0; i < case_count; i++) print_case(&sorted[i]);

    build_graph();
    puts("");
    bfs(2); /* Start at C103 */
    dfs_components();

    printf("\nLCS length for two keyword sequences: %d\n",
           lcs_length(seq_a, 3, seq_b, 3));
    puts("\nNote: shared attributes and keyword matches are not proof of a connection.");
    free_hash_index();
    return 0;
}
