#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef long long ll;

typedef struct {
    ll arrival;
    int track;
} Request;

/* Pending requests are stored in a treap ordered by (track, input order).
   This gives us fast insert/remove and lets each algorithm find nearby
   tracks efficiently. */

typedef struct Node {
    int id;
    int track;
    unsigned int prio;
    struct Node *left;
    struct Node *right;
} Node;

static Node *treap_root = NULL;
static Node *nodes = NULL;

/* Simple xorshift RNG used to assign treap priorities. */
static unsigned int rng_state = 2463534242u;
static unsigned int next_rand_u32(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static int key_less_pair(int track1, int id1, int track2, int id2) {
    if (track1 != track2) return track1 < track2;
    return id1 < id2;
}

/* Split the treap into keys < (track, id) and keys >= (track, id). */
static void split(Node *root, int track, int id, Node **left, Node **right) {
    if (root == NULL) {
        *left = NULL;
        *right = NULL;
        return;
    }

    if (key_less_pair(root->track, root->id, track, id)) {
        split(root->right, track, id, &root->right, right);
        *left = root;
    } else {
        split(root->left, track, id, left, &root->left);
        *right = root;
    }
}

static Node *merge(Node *left, Node *right) {
    if (!left) return right;
    if (!right) return left;

    if (left->prio > right->prio) {
        left->right = merge(left->right, right);
        return left;
    } else {
        right->left = merge(left, right->left);
        return right;
    }
}

/* Standard treap insert: keep heap order by priority and BST order by key. */
static Node *insert_node(Node *root, Node *node) {
    if (!root) return node;

    if (node->prio > root->prio) {
        split(root, node->track, node->id, &node->left, &node->right);
        return node;
    }

    if (key_less_pair(node->track, node->id, root->track, root->id)) {
        root->left = insert_node(root->left, node);
    } else {
        root->right = insert_node(root->right, node);
    }
    return root;
}

/* Remove one specific request from the pending set. */
static Node *erase_node(Node *root, int track, int id) {
    if (!root) return NULL;

    if (root->track == track && root->id == id) {
        Node *merged = merge(root->left, root->right);
        root->left = root->right = NULL;
        return merged;
    }

    if (key_less_pair(track, id, root->track, root->id)) {
        root->left = erase_node(root->left, track, id);
    } else {
        root->right = erase_node(root->right, track, id);
    }
    return root;
}

static Node *find_min(Node *root) {
    if (!root) return NULL;
    while (root->left) root = root->left;
    return root;
}

static Node *find_max(Node *root) {
    if (!root) return NULL;
    while (root->right) root = root->right;
    return root;
}

/* first node with key >= (track, id) */
static Node *lower_bound_node(Node *root, int track, int id) {
    Node *ans = NULL;
    while (root) {
        if (!key_less_pair(root->track, root->id, track, id)) {
            ans = root;
            root = root->left;
        } else {
            root = root->right;
        }
    }
    return ans;
}

/* last node with key <= (track, id) */
static Node *upper_bound_prev_node(Node *root, int track, int id) {
    Node *ans = NULL;
    while (root) {
        if (!key_less_pair(track, id, root->track, root->id)) {
            ans = root;
            root = root->right;
        } else {
            root = root->left;
        }
    }
    return ans;
}

/* ---------- Utility ---------- */

static ll llabs_diff(int a, int b) {
    return (a >= b) ? (ll)(a - b) : (ll)(b - a);
}

static int is_directional_alg(const char *alg) {
    return strcmp(alg, "SCAN") == 0 ||
           strcmp(alg, "C-SCAN") == 0 ||
           strcmp(alg, "LOOK") == 0 ||
           strcmp(alg, "C-LOOK") == 0;
}

static int parse_direction(const char *dir_str) {
    return (strcmp(dir_str, "UP") == 0) ? 1 : -1;
}

/* For FCFS, newly arrived requests are appended in input order. */
static void add_arrivals_fcfs(const Request *reqs, int R, int *next_idx,
                              ll current_time, int *queue, int *qt) {
    while (*next_idx < R && reqs[*next_idx].arrival <= current_time) {
        queue[(*qt)++] = *next_idx;
        (*next_idx)++;
    }
}

/* For the other algorithms, newly arrived requests enter the ordered treap. */
static void add_arrivals_treap(const Request *reqs, int R, int *next_idx,
                               ll current_time) {
    while (*next_idx < R && reqs[*next_idx].arrival <= current_time) {
        int id = *next_idx;
        nodes[id].id = id;
        nodes[id].track = reqs[id].track;
        nodes[id].prio = next_rand_u32();
        nodes[id].left = NULL;
        nodes[id].right = NULL;
        treap_root = insert_node(treap_root, &nodes[id]);
        (*next_idx)++;
    }
}

/* ---------- Main ---------- */

int main(void) {
    char first_line[128];
    if (!fgets(first_line, sizeof(first_line), stdin)) {
        return 0;
    }

    char alg[32];
    char dir_str[16];
    int N, H;
    int direction = 0;

    if (sscanf(first_line, "%31s %d %d %15s", alg, &N, &H, dir_str) < 3) {
        return 0;
    }
    if (is_directional_alg(alg)) {
        direction = parse_direction(dir_str);
    }

    int R;
    if (scanf("%d", &R) != 1) {
        return 0;
    }

    Request *reqs = (Request *)malloc((size_t)R * sizeof(Request));
    int *order = (int *)malloc((size_t)R * sizeof(int));
    if (!reqs || !order) {
        free(reqs);
        free(order);
        return 0;
    }

    for (int i = 0; i < R; i++) {
        scanf("%lld %d", &reqs[i].arrival, &reqs[i].track);
    }

    ll current_time = 0;
    ll total_seek = 0;
    int head = H;
    int served_count = 0;
    int next_idx = 0;

    /* FCFS serves requests strictly in arrival/input order once they are ready. */
    if (strcmp(alg, "FCFS") == 0) {
        int *queue = (int *)malloc((size_t)R * sizeof(int));
        if (!queue) {
            free(reqs);
            free(order);
            return 0;
        }

        int qh = 0, qt = 0;

        while (served_count < R) {
            add_arrivals_fcfs(reqs, R, &next_idx, current_time, queue, &qt);

            if (qh == qt) {
                /* If nothing is pending, jump forward to the next arrival time. */
                current_time = reqs[next_idx].arrival;
                add_arrivals_fcfs(reqs, R, &next_idx, current_time, queue, &qt);
            }

            int id = queue[qh++];
            int target = reqs[id].track;
            ll dist = llabs_diff(head, target);

            total_seek += dist;
            current_time += dist;
            head = target;

            order[served_count++] = target;
        }

        free(queue);
    }

    /* All remaining algorithms choose from a set of currently pending requests. */
    else {
        nodes = (Node *)malloc((size_t)R * sizeof(Node));
        if (!nodes) {
            free(reqs);
            free(order);
            return 0;
        }

        treap_root = NULL;

        while (served_count < R) {
            add_arrivals_treap(reqs, R, &next_idx, current_time);

            if (treap_root == NULL) {
                /* No request is available yet, so fast-forward to the next arrival. */
                current_time = reqs[next_idx].arrival;
                add_arrivals_treap(reqs, R, &next_idx, current_time);
            }

            /* SSTF picks the pending request closest to the current head. */
            if (strcmp(alg, "SSTF") == 0) {
                Node *succ = lower_bound_node(treap_root, head, -1);
                Node *pred = upper_bound_prev_node(treap_root, head, INT_MAX);

                Node *chosen = NULL;
                if (!succ) {
                    chosen = pred;
                } else if (!pred) {
                    chosen = succ;
                } else {
                    ll d1 = llabs_diff(head, pred->track);
                    ll d2 = llabs_diff(head, succ->track);

                    if (d1 < d2) chosen = pred;
                    else if (d2 < d1) chosen = succ;
                    else {
                        /* tie: smaller track first; same track -> smaller input id */
                        if (pred->track < succ->track) chosen = pred;
                        else if (succ->track < pred->track) chosen = succ;
                        else chosen = (pred->id < succ->id) ? pred : succ;
                    }
                }

                int target = chosen->track;
                ll dist = llabs_diff(head, target);

                total_seek += dist;
                current_time += dist;
                head = target;

                order[served_count++] = target;
                treap_root = erase_node(treap_root, chosen->track, chosen->id);
            }

            /* LOOK moves in one direction until no more requests remain there,
               then reverses without going all the way to the disk edge. */
            else if (strcmp(alg, "LOOK") == 0) {
                Node *chosen = NULL;

                if (direction == 1) {
                    chosen = lower_bound_node(treap_root, head, -1);
                    if (!chosen) {
                        direction = -1;
                        chosen = find_max(treap_root);
                    }
                } else {
                    chosen = upper_bound_prev_node(treap_root, head, INT_MAX);
                    if (!chosen) {
                        direction = 1;
                        chosen = find_min(treap_root);
                    }
                }

                int target = chosen->track;
                ll dist = llabs_diff(head, target);

                total_seek += dist;
                current_time += dist;
                head = target;

                order[served_count++] = target;
                treap_root = erase_node(treap_root, chosen->track, chosen->id);
            }

            /* SCAN moves toward a disk edge, reverses there, and continues. */
            else if (strcmp(alg, "SCAN") == 0) {
                Node *chosen = NULL;

                if (direction == 1) {
                    chosen = lower_bound_node(treap_root, head, -1);
                    if (!chosen) {
                        /* No request ahead: finish the sweep to the top edge first. */
                        if (head != N - 1) {
                            ll dist = (ll)(N - 1 - head);
                            total_seek += dist;
                            current_time += dist;
                            head = N - 1;
                        }
                        direction = -1;
                        continue;
                    }
                } else {
                    chosen = upper_bound_prev_node(treap_root, head, INT_MAX);
                    if (!chosen) {
                        /* No request behind: finish the sweep to track 0 first. */
                        if (head != 0) {
                            ll dist = (ll)head;
                            total_seek += dist;
                            current_time += dist;
                            head = 0;
                        }
                        direction = 1;
                        continue;
                    }
                }

                int target = chosen->track;
                ll dist = llabs_diff(head, target);

                total_seek += dist;
                current_time += dist;
                head = target;

                order[served_count++] = target;
                treap_root = erase_node(treap_root, chosen->track, chosen->id);
            }

            /* C-SCAN sweeps in one direction only, then wraps to the far edge. */
            else if (strcmp(alg, "C-SCAN") == 0) {
                Node *chosen = NULL;

                if (direction == 1) {
                    chosen = lower_bound_node(treap_root, head, -1);
                    if (!chosen) {
                        /* Go to the top edge, then wrap to 0 and keep scanning up. */
                        if (head != N - 1) {
                            ll dist = (ll)(N - 1 - head);
                            total_seek += dist;
                            current_time += dist;
                            head = N - 1;
                        }
                        if (head != 0) {
                            ll dist = (ll)(N - 1);
                            total_seek += dist;
                            current_time += dist;
                            head = 0;
                        }
                        continue;
                    }
                } else {
                    chosen = upper_bound_prev_node(treap_root, head, INT_MAX);
                    if (!chosen) {
                        /* Go to 0, then wrap to the top edge and keep scanning down. */
                        if (head != 0) {
                            ll dist = (ll)head;
                            total_seek += dist;
                            current_time += dist;
                            head = 0;
                        }
                        if (head != N - 1) {
                            ll dist = (ll)(N - 1);
                            total_seek += dist;
                            current_time += dist;
                            head = N - 1;
                        }
                        continue;
                    }
                }

                int target = chosen->track;
                ll dist = llabs_diff(head, target);

                total_seek += dist;
                current_time += dist;
                head = target;

                order[served_count++] = target;
                treap_root = erase_node(treap_root, chosen->track, chosen->id);
            }

            /* C-LOOK wraps directly to the furthest pending request
               instead of traveling to the physical disk edge. */
            else if (strcmp(alg, "C-LOOK") == 0) {
                Node *chosen = NULL;

                if (direction == 1) {
                    chosen = lower_bound_node(treap_root, head, -1);
                    if (!chosen) {
                        chosen = find_min(treap_root);
                    }
                } else {
                    chosen = upper_bound_prev_node(treap_root, head, INT_MAX);
                    if (!chosen) {
                        chosen = find_max(treap_root);
                    }
                }

                int target = chosen->track;
                ll dist = llabs_diff(head, target);

                total_seek += dist;
                current_time += dist;
                head = target;

                order[served_count++] = target;
                treap_root = erase_node(treap_root, chosen->track, chosen->id);
            }
        }

        free(nodes);
    }

    /* Output */
    for (int i = 0; i < R; i++) {
        if (i) printf(" ");
        printf("%d", order[i]);
    }
    printf("\n%lld\n%lld\n", total_seek, current_time);

    free(reqs);
    free(order);
    return 0;
}
