/*
 * spireslayer - CMPE 230 Assignment 1
 *
 * Reads one command per line, updates the game state, prints the response.
 * Each line is split into tokens, then we look at the first few words to
 * figure out which command it is. Everything is kept in plain global arrays
 * so we don't have to deal with malloc/free.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1100
#define MAXTOK  600
#define MAXNAME 1100
#define MAXENT  4096

/* These words can never be used as part of a name. */
static const char *RESERVED[] = {
    "Ironclad","gains","gold","max","hp","card","relic","potion","buys","for",
    "removes","upgraded","upgrades","enters","room","learns","is","effective",
    "against","fights","heals","takes","damage","discards","sells","marks","as",
    "exhaust","Total","Floor","Where","Deck","size","Relics","Potions","What",
    "Defeated","Health","Exhausts","Exit", NULL
};

/* Is this word one of the reserved keywords? */
static int is_reserved(const char *w) {
    for (int i = 0; RESERVED[i]; i++)
        if (!strcmp(RESERVED[i], w)) return 1;
    return 0;
}

/* True if the word is non-empty and only contains A-Z / a-z. */
static int is_letter_word(const char *w) {
    if (!*w) return 0;
    for (const char *p = w; *p; p++)
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z'))) return 0;
    return 1;
}

/* A word is allowed inside a name only if it's letters and not a keyword. */
static int is_name_word(const char *w) {
    return is_letter_word(w) && !is_reserved(w);
}

/* Parse a positive integer. No leading zero, no sign, must fit in int. */
static int parse_pos_int(const char *w, int *out) {
    if (!*w || w[0] == '0') return 0;
    long v = 0;
    for (const char *p = w; *p; p++) {
        if (*p < '0' || *p > '9') return 0;
        v = v * 10 + (*p - '0');
        if (v > 2147483647L) return 0;
    }
    if (v < 1) return 0;
    *out = (int)v;
    return 1;
}

/* Split the line into tokens by spaces. We also remember how many spaces
 * came before each token in gaps[], because names need exactly one space
 * between their words while everywhere else any number of spaces is fine.
 */
static int gaps[MAXTOK];
static int tokenize(const char *line, char tok[][MAXNAME], int *ntok) {
    int n = 0, i = 0;
    while (line[i]) {
        int g = 0;
        while (line[i] == ' ') { i++; g++; }
        if (!line[i]) break;
        gaps[n] = g;
        int j = 0;
        while (line[i] && line[i] != ' ') {
            if (j >= MAXNAME - 1) return 0;
            tok[n][j++] = line[i++];
        }
        tok[n][j] = '\0';
        if (++n >= MAXTOK) return 0;
    }
    *ntok = n;
    return 1;
}

/* Glue tokens[a..b-1] back into a name like "Shrug It Off".
 * Returns 0 if the range is empty or any word isn't a valid name word.
 */
static int join_name(char tok[][MAXNAME], int a, int b, char *out) {
    if (b <= a) return 0;
    int len = 0;
    for (int i = a; i < b; i++) {
        if (!is_name_word(tok[i])) return 0;
        if (i > a) {
            /* Only one space allowed between two words of the same name. */
            if (gaps[i] != 1) return 0;
            out[len++] = ' ';
        }
        int l = (int)strlen(tok[i]);
        memcpy(out + len, tok[i], l);
        len += l;
    }
    out[len] = '\0';
    return 1;
}

/* ===================== Game state ===================== */
/* A card keeps two counters: how many normal copies and how many upgraded. */
typedef struct { char name[MAXNAME]; int base, upg; } Card;
typedef struct { char name[MAXNAME]; } Relic;
typedef struct { char name[MAXNAME]; int count; } Potion;
/* type is just a single char: 'c'=card, 'r'=relic, 'p'=potion. */
typedef struct { char type; char name[MAXNAME]; } Counter;
typedef struct {
    char enemy[MAXNAME];
    Counter cs[128];
    int nc;
    int defeated;
} Codex;

static Card  cards[MAXENT];   static int ncards   = 0;
static char  exhausts[MAXENT][MAXNAME]; static int nexh = 0;
static Relic relics[MAXENT];  static int nrelics  = 0;
static Potion potions[8];     static int npotions = 0;
static Codex codex[MAXENT];   static int ncodex   = 0;
static int   gold = 0, hp = 80, maxhp = 80, floor_ = 0;
static char  current_room[16] = "NONE";

/* Look up a card by name. Returns -1 if we've never seen it. */
static int find_card(const char *n) {
    for (int i = 0; i < ncards; i++) if (!strcmp(cards[i].name, n)) return i;
    return -1;
}
/* Same as find_card but adds an empty entry if it's missing. */
static int ensure_card(const char *n) {
    int i = find_card(n);
    if (i >= 0) return i;
    strcpy(cards[ncards].name, n);
    cards[ncards].base = cards[ncards].upg = 0;
    return ncards++;
}
/* Has this card name been tagged as exhaust? */
static int is_exhaust(const char *n) {
    for (int i = 0; i < nexh; i++) if (!strcmp(exhausts[i], n)) return 1;
    return 0;
}
/* Look up a relic by name, -1 if we don't own it. */
static int find_relic(const char *n) {
    for (int i = 0; i < nrelics; i++) if (!strcmp(relics[i].name, n)) return i;
    return -1;
}
/* Look up a potion slot in the belt, -1 if not there. */
static int find_potion(const char *n) {
    for (int i = 0; i < npotions; i++) if (!strcmp(potions[i].name, n)) return i;
    return -1;
}
/* How many potions are in the belt right now (counts duplicates). */
static int total_potions(void) {
    int s = 0;
    for (int i = 0; i < npotions; i++) s += potions[i].count;
    return s;
}
/* Look up a codex entry for an enemy, -1 if there isn't one yet. */
static int find_codex(const char *n) {
    for (int i = 0; i < ncodex; i++) if (!strcmp(codex[i].enemy, n)) return i;
    return -1;
}
/* find_codex but creates an empty entry if missing. */
static int ensure_codex(const char *n) {
    int i = find_codex(n);
    if (i >= 0) return i;
    strcpy(codex[ncodex].enemy, n);
    codex[ncodex].nc = 0;
    codex[ncodex].defeated = 0;
    return ncodex++;
}
/* If a potion slot just hit 0, drop it from the array so we don't keep
 * empty entries lying around. */
static void compact_potion(int i) {
    if (potions[i].count == 0) {
        for (int j = i; j < npotions - 1; j++) potions[j] = potions[j + 1];
        npotions--;
    }
}

/* Comparators for qsort - sort everything alphabetically. */
static int cmpstr(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}
static int cmpcard(const void *a, const void *b) {
    return strcmp(((const Card *)a)->name, ((const Card *)b)->name);
}
static int cmppot(const void *a, const void *b) {
    return strcmp(((const Potion *)a)->name, ((const Potion *)b)->name);
}

/* ===================== Commands that change state ===================== */

/* Add gold to the total. */
static void cmd_gain_gold(int n) { gold += n; printf("Gold obtained\n"); }

/* Add one base copy of a card to the deck. */
static void cmd_gain_card(const char *n) {
    int i = ensure_card(n); cards[i].base++;
    printf("Card added: %s\n", n);
}

/* Pick up a relic, or do nothing if we already own it. */
static void cmd_gain_relic(const char *n) {
    if (find_relic(n) >= 0) { printf("Already has relic: %s\n", n); return; }
    strcpy(relics[nrelics++].name, n);
    printf("Relic obtained: %s\n", n);
}
/* Tries to put one potion in the belt. Returns 0 if there's no room. */
static int try_gain_potion(const char *n) {
    if (total_potions() >= 3) return 0;
    int i = find_potion(n);
    if (i < 0) {
        strcpy(potions[npotions].name, n);
        potions[npotions].count = 1;
        npotions++;
    } else {
        potions[i].count++;
    }
    return 1;
}
/* Wrapper around try_gain_potion that prints the right message. */
static void cmd_gain_potion(const char *n) {
    if (try_gain_potion(n)) printf("Potion obtained: %s\n", n);
    else printf("Potion belt is full\n");
}

/* Buying: check gold first, then check the item rule, then deduct. */
static void cmd_buy_card(const char *n, int p) {
    if (gold < p) { printf("Not enough gold\n"); return; }
    gold -= p;
    cmd_gain_card(n);
}
static void cmd_buy_relic(const char *n, int p) {
    if (gold < p) { printf("Not enough gold\n"); return; }
    if (find_relic(n) >= 0) { printf("Already has relic: %s\n", n); return; }
    gold -= p;
    strcpy(relics[nrelics++].name, n);
    printf("Relic obtained: %s\n", n);
}
static void cmd_buy_potion(const char *n, int p) {
    if (gold < p) { printf("Not enough gold\n"); return; }
    if (total_potions() >= 3) { printf("Potion belt is full\n"); return; }
    gold -= p;
    try_gain_potion(n);
    printf("Potion obtained: %s\n", n);
}

/* Remove one base copy of a card from the deck. */
static void cmd_remove_card(const char *n) {
    int i = find_card(n);
    if (i < 0 || cards[i].base <= 0) { printf("Card not found: %s\n", n); return; }
    cards[i].base--;
    printf("Card removed: %s\n", n);
}
/* Remove one upgraded copy of a card. */
static void cmd_remove_upg(const char *n) {
    int i = find_card(n);
    if (i < 0 || cards[i].upg <= 0) { printf("Upgraded card not found: %s\n", n); return; }
    cards[i].upg--;
    printf("Upgraded card removed: %s\n", n);
}
/* Turn one base copy into an upgraded copy. */
static void cmd_upgrade_card(const char *n) {
    int i = find_card(n);
    if (i < 0 || cards[i].base <= 0) { printf("Card not found: %s\n", n); return; }
    cards[i].base--;
    cards[i].upg++;
    printf("Card upgraded: %s\n", n);
}

/* Move to a new room and bump the floor counter. */
static void cmd_enter(const char *r) {
    floor_++;
    strncpy(current_room, r, sizeof(current_room) - 1);
    current_room[sizeof(current_room) - 1] = '\0';
    printf("Entered %s room\n", r);
}

/* Record a new effectiveness note in the codex for an enemy. */
static void cmd_learn(char type, const char *item, const char *enemy) {
    int existed = find_codex(enemy) >= 0;
    int i = ensure_codex(enemy);
    for (int k = 0; k < codex[i].nc; k++)
        if (codex[i].cs[k].type == type && !strcmp(codex[i].cs[k].name, item)) {
            printf("Effectiveness already noted\n");
            return;
        }
    codex[i].cs[codex[i].nc].type = type;
    strcpy(codex[i].cs[codex[i].nc].name, item);
    codex[i].nc++;
    printf("Codex entry %s: %s\n", existed ? "updated" : "created", enemy);
}

/* True if this codex counter is something we can actually use right now. */
static int counter_available(Counter *c) {
    if (c->type == 'c') {
        int i = find_card(c->name);
        return i >= 0 && (cards[i].base + cards[i].upg) > 0;
    }
    if (c->type == 'r') return find_relic(c->name) >= 0;
    if (c->type == 'p') {
        int i = find_potion(c->name);
        return i >= 0 && potions[i].count > 0;
    }
    return 0;
}

/* Runs a fight. Returns 1 if we win, 0 if we lose. Handles all side effects
 * (using up potions, removing exhaust cards, taking damage, etc.). */
static int do_fight(const char *enemy) {
    int ci = find_codex(enemy);
    int won = 0;
    /* We win if we have at least one effective counter ready to use. */
    if (ci >= 0) {
        for (int k = 0; k < codex[ci].nc; k++)
            if (counter_available(&codex[ci].cs[k])) { won = 1; break; }
    }
    if (!won) {
        hp -= 15; if (hp < 0) hp = 0;
        return 0;
    }
    /* Use up one copy of every effective potion we still have. */
    for (int k = 0; k < codex[ci].nc; k++) {
        Counter *c = &codex[ci].cs[k];
        if (c->type == 'p') {
            int i = find_potion(c->name);
            if (i >= 0 && potions[i].count > 0) {
                potions[i].count--;
                compact_potion(i);
            }
        }
    }
    /* Exhaust cards that worked also lose one copy. Drop a base copy first,
     * fall back to an upgraded one if there's no base. */
    for (int k = 0; k < codex[ci].nc; k++) {
        Counter *c = &codex[ci].cs[k];
        if (c->type == 'c' && is_exhaust(c->name)) {
            int i = find_card(c->name);
            if (i >= 0) {
                if (cards[i].base > 0) cards[i].base--;
                else if (cards[i].upg > 0) cards[i].upg--;
            }
        }
    }
    codex[ci].defeated++;
    return 1;
}
/* Plain "Ironclad fights X" command. */
static void cmd_fight(const char *e) {
    if (do_fight(e)) printf("Ironclad defeats %s\n", e);
    else printf("Ironclad is outmatched and flees with %d hp remaining\n", hp);
}
/* Same as cmd_fight but with a gold reward on victory. */
static void cmd_fight_bounty(const char *e, int g) {
    if (do_fight(e)) {
        gold += g;
        printf("Ironclad defeats %s and gains %d gold\n", e, g);
    } else {
        printf("Ironclad is outmatched and flees with %d hp remaining\n", hp);
    }
}

/* Heal up, but never above max hp. */
static void cmd_heal(int n) {
    hp += n; if (hp > maxhp) hp = maxhp;
    printf("Ironclad heals to %d\n", hp);
}
/* Take damage, never go below 0. */
static void cmd_take(int n) {
    hp -= n; if (hp < 0) hp = 0;
    printf("Ironclad health drops to %d\n", hp);
}
/* Throw away one potion from the belt. */
static void cmd_discard(const char *n) {
    int i = find_potion(n);
    if (i < 0 || potions[i].count <= 0) {
        printf("Potion not found: %s\n", n); return;
    }
    potions[i].count--;
    compact_potion(i);
    printf("Potion discarded: %s\n", n);
}

/* Selling: remove the item if we have it, then add the price to our gold. */
static void cmd_sell_card(const char *n, int p) {
    int i = find_card(n);
    if (i < 0 || cards[i].base <= 0) { printf("Card not found: %s\n", n); return; }
    cards[i].base--; gold += p;
    printf("Card sold: %s\n", n);
}
static void cmd_sell_upg(const char *n, int p) {
    int i = find_card(n);
    if (i < 0 || cards[i].upg <= 0) { printf("Upgraded card not found: %s\n", n); return; }
    cards[i].upg--; gold += p;
    printf("Upgraded card sold: %s\n", n);
}
static void cmd_sell_potion(const char *n, int p) {
    int i = find_potion(n);
    if (i < 0 || potions[i].count <= 0) { printf("Potion not found: %s\n", n); return; }
    potions[i].count--;
    compact_potion(i);
    gold += p;
    printf("Potion sold: %s\n", n);
}

/* Tag a card name as exhaust (the tag sticks forever). */
static void cmd_mark_exhaust(const char *n) {
    if (is_exhaust(n)) { printf("Card already exhausts: %s\n", n); return; }
    strcpy(exhausts[nexh++], n);
    printf("Card marked as exhaust: %s\n", n);
}
/* Increase the max hp cap. Current hp stays the same. */
static void cmd_gain_max_hp(int n) {
    maxhp += n;
    printf("Max health increased to %d\n", maxhp);
}

/* ===================== Read-only queries ===================== */

/* Print current gold. */
static void q_gold(void)  { printf("%d\n", gold);   }
/* Print current floor number. */
static void q_floor(void) { printf("%d\n", floor_); }
/* Print current room name (or NONE if we never entered one). */
static void q_where(void) { printf("%s\n", current_room); }
/* Print total copies of a card (base + upgraded). */
static void q_total_card(const char *n) {
    int i = find_card(n);
    printf("%d\n", i < 0 ? 0 : cards[i].base + cards[i].upg);
}
/* Print only the upgraded copies of a card. */
static void q_total_upg(const char *n) {
    int i = find_card(n);
    printf("%d\n", i < 0 ? 0 : cards[i].upg);
}
static void q_deck(void) {
    /* Copy out only the non-empty entries, sort them, then print. Base
     * versions go before upgraded ones for the same card. */
    Card sorted[MAXENT]; int m = 0;
    for (int i = 0; i < ncards; i++)
        if (cards[i].base + cards[i].upg > 0) sorted[m++] = cards[i];
    if (m == 0) { printf("None\n"); return; }
    qsort(sorted, m, sizeof(Card), cmpcard);
    int first = 1;
    for (int i = 0; i < m; i++) {
        int ex = is_exhaust(sorted[i].name);
        if (sorted[i].base > 0) {
            if (!first) printf(", ");
            first = 0;
            printf("%d %s%s", sorted[i].base, sorted[i].name, ex ? "*" : "");
        }
        if (sorted[i].upg > 0) {
            if (!first) printf(", ");
            first = 0;
            printf("%d %s+%s", sorted[i].upg, sorted[i].name, ex ? "*" : "");
        }
    }
    printf("\n");
}
/* Print all owned relics, sorted alphabetically. */
static void q_relics(void) {
    if (nrelics == 0) { printf("None\n"); return; }
    static char arr[MAXENT][MAXNAME];
    for (int i = 0; i < nrelics; i++) strcpy(arr[i], relics[i].name);
    qsort(arr, nrelics, MAXNAME, cmpstr);
    for (int i = 0; i < nrelics; i++) {
        if (i) printf(", ");
        printf("%s", arr[i]);
    }
    printf("\n");
}
/* Print every potion in the belt with its count, sorted by name. */
static void q_potions(void) {
    int total = total_potions();
    if (total == 0) { printf("None\n"); return; }
    Potion s[8]; int m = 0;
    for (int i = 0; i < npotions; i++) if (potions[i].count > 0) s[m++] = potions[i];
    qsort(s, m, sizeof(Potion), cmppot);
    for (int i = 0; i < m; i++) {
        if (i) printf(", ");
        printf("%d %s", s[i].count, s[i].name);
    }
    printf("\n");
}
/* Print everything we know is effective against a given enemy. */
static void q_effective(const char *e) {
    int i = find_codex(e);
    if (i < 0 || codex[i].nc == 0) { printf("No codex data for %s\n", e); return; }
    /* Build the printable form of every counter first ("card Bash" etc.),
     * then sort the whole strings so the ordering matches the spec. */
    static char lines[128][MAXNAME + 16];
    int n = codex[i].nc;
    for (int k = 0; k < n; k++) {
        const char *t = codex[i].cs[k].type == 'c' ? "card"
                       : codex[i].cs[k].type == 'r' ? "relic" : "potion";
        snprintf(lines[k], sizeof(lines[k]), "%s %s", t, codex[i].cs[k].name);
    }
    qsort(lines, n, sizeof(lines[0]), cmpstr);
    for (int k = 0; k < n; k++) {
        if (k) printf(", ");
        printf("%s", lines[k]);
    }
    printf("\n");
}
/* How many times we've beaten an enemy. */
static void q_defeated(const char *e) {
    int i = find_codex(e);
    printf("%d\n", i < 0 ? 0 : codex[i].defeated);
}
/* Print "current/max" hp. */
static void q_health(void) { printf("%d/%d\n", hp, maxhp); }
/* Total number of cards in the deck (base + upgraded across all names). */
static void q_deck_size(void) {
    int s = 0;
    for (int i = 0; i < ncards; i++) s += cards[i].base + cards[i].upg;
    printf("%d\n", s);
}
/* Print all card names tagged as exhaust. */
static void q_exhausts(void) {
    if (nexh == 0) { printf("None\n"); return; }
    static char arr[MAXENT][MAXNAME];
    for (int i = 0; i < nexh; i++) strcpy(arr[i], exhausts[i]);
    qsort(arr, nexh, MAXNAME, cmpstr);
    for (int i = 0; i < nexh; i++) {
        if (i) printf(", ");
        printf("%s", arr[i]);
    }
    printf("\n");
}

/* Looks at the tokens and figures out which command (if any) we have.
 * Returns 1 if we handled it (and already printed something), 2 for Exit,
 * 0 if nothing matched so the caller should print INVALID. */
static int dispatch(char tok[][MAXNAME], int n) {
    if (n == 0) return 0;
    if (n == 1 && !strcmp(tok[0], "Exit")) return 2;

    /* Queries always end with a "?" token. */
    if (n >= 2 && !strcmp(tok[n - 1], "?")) {
        if (n == 3 && !strcmp(tok[0], "Total") && !strcmp(tok[1], "gold")) { q_gold(); return 1; }
        if (n == 2 && !strcmp(tok[0], "Floor")) { q_floor(); return 1; }
        if (n == 2 && !strcmp(tok[0], "Where")) { q_where(); return 1; }
        if (n >= 5 && !strcmp(tok[0], "Total") && !strcmp(tok[1], "upgraded") && !strcmp(tok[2], "card")) {
            char nm[MAXNAME];
            if (!join_name(tok, 3, n - 1, nm)) return 0;
            q_total_upg(nm); return 1;
        }
        if (n >= 4 && !strcmp(tok[0], "Total") && !strcmp(tok[1], "card")) {
            char nm[MAXNAME];
            if (!join_name(tok, 2, n - 1, nm)) return 0;
            q_total_card(nm); return 1;
        }
        if (n == 2 && !strcmp(tok[0], "Deck")) { q_deck(); return 1; }
        if (n == 3 && !strcmp(tok[0], "Deck") && !strcmp(tok[1], "size")) { q_deck_size(); return 1; }
        if (n == 2 && !strcmp(tok[0], "Relics")) { q_relics(); return 1; }
        if (n == 2 && !strcmp(tok[0], "Potions")) { q_potions(); return 1; }
        if (n >= 6 && !strcmp(tok[0], "What") && !strcmp(tok[1], "is") &&
            !strcmp(tok[2], "effective") && !strcmp(tok[3], "against")) {
            char nm[MAXNAME];
            if (!join_name(tok, 4, n - 1, nm)) return 0;
            q_effective(nm); return 1;
        }
        if (n >= 3 && !strcmp(tok[0], "Defeated")) {
            char nm[MAXNAME];
            if (!join_name(tok, 1, n - 1, nm)) return 0;
            q_defeated(nm); return 1;
        }
        if (n == 2 && !strcmp(tok[0], "Health")) { q_health(); return 1; }
        if (n == 2 && !strcmp(tok[0], "Exhausts")) { q_exhausts(); return 1; }
        return 0;
    }

    /* Everything else has to start with "Ironclad ...". */
    if (strcmp(tok[0], "Ironclad")) return 0;
    if (n < 3) return 0;
    const char *verb = tok[1];

    if (!strcmp(verb, "gains")) {
        if (n >= 4 && !strcmp(tok[2], "card")) {
            char nm[MAXNAME];
            if (!join_name(tok, 3, n, nm)) return 0;
            cmd_gain_card(nm); return 1;
        }
        if (n >= 4 && !strcmp(tok[2], "relic")) {
            char nm[MAXNAME];
            if (!join_name(tok, 3, n, nm)) return 0;
            cmd_gain_relic(nm); return 1;
        }
        if (n >= 4 && !strcmp(tok[2], "potion")) {
            char nm[MAXNAME];
            if (!join_name(tok, 3, n, nm)) return 0;
            cmd_gain_potion(nm); return 1;
        }
        int v;
        if (n == 4 && parse_pos_int(tok[2], &v) && !strcmp(tok[3], "gold")) {
            cmd_gain_gold(v); return 1;
        }
        if (n == 5 && parse_pos_int(tok[2], &v) && !strcmp(tok[3], "max") && !strcmp(tok[4], "hp")) {
            cmd_gain_max_hp(v); return 1;
        }
        return 0;
    }

    if (!strcmp(verb, "buys")) {
        if (n < 7) return 0;
        if (strcmp(tok[n - 1], "gold")) return 0;
        int v;
        if (!parse_pos_int(tok[n - 2], &v)) return 0;
        if (strcmp(tok[n - 3], "for")) return 0;
        char nm[MAXNAME];
        if (!strcmp(tok[2], "card")) {
            if (!join_name(tok, 3, n - 3, nm)) return 0;
            cmd_buy_card(nm, v); return 1;
        }
        if (!strcmp(tok[2], "relic")) {
            if (!join_name(tok, 3, n - 3, nm)) return 0;
            cmd_buy_relic(nm, v); return 1;
        }
        if (!strcmp(tok[2], "potion")) {
            if (!join_name(tok, 3, n - 3, nm)) return 0;
            cmd_buy_potion(nm, v); return 1;
        }
        return 0;
    }

    if (!strcmp(verb, "removes")) {
        if (n >= 5 && !strcmp(tok[2], "upgraded") && !strcmp(tok[3], "card")) {
            char nm[MAXNAME];
            if (!join_name(tok, 4, n, nm)) return 0;
            cmd_remove_upg(nm); return 1;
        }
        if (n >= 4 && !strcmp(tok[2], "card")) {
            char nm[MAXNAME];
            if (!join_name(tok, 3, n, nm)) return 0;
            cmd_remove_card(nm); return 1;
        }
        return 0;
    }

    if (!strcmp(verb, "upgrades")) {
        if (n >= 4 && !strcmp(tok[2], "card")) {
            char nm[MAXNAME];
            if (!join_name(tok, 3, n, nm)) return 0;
            cmd_upgrade_card(nm); return 1;
        }
        return 0;
    }

    if (!strcmp(verb, "enters")) {
        if (n != 4) return 0;
        if (strcmp(tok[3], "room")) return 0;
        const char *r = tok[2];
        if (strcmp(r, "Monster") && strcmp(r, "Elite") && strcmp(r, "Rest") &&
            strcmp(r, "Shop") && strcmp(r, "Treasure") && strcmp(r, "Event") &&
            strcmp(r, "Boss")) return 0;
        cmd_enter(r); return 1;
    }

    if (!strcmp(verb, "learns")) {
        if (n < 8) return 0;
        char type;
        if (!strcmp(tok[2], "card"))   type = 'c';
        else if (!strcmp(tok[2], "relic"))  type = 'r';
        else if (!strcmp(tok[2], "potion")) type = 'p';
        else return 0;
        /* Find where "is effective against" starts; the item name is
         * everything between the type word and that point. */
        int p = -1;
        for (int i = 4; i + 3 <= n - 1; i++) {
            if (!strcmp(tok[i], "is") && !strcmp(tok[i + 1], "effective") &&
                !strcmp(tok[i + 2], "against")) { p = i; break; }
        }
        if (p < 0) return 0;
        char item[MAXNAME], enemy[MAXNAME];
        if (!join_name(tok, 3, p, item)) return 0;
        if (!join_name(tok, p + 3, n, enemy)) return 0;
        cmd_learn(type, item, enemy); return 1;
    }

    if (!strcmp(verb, "fights")) {
        if (n < 3) return 0;
        int v;
        /* "Ironclad fights X for N gold" - the bounty version. Check the
         * tail first; if it doesn't fit we treat it as a normal fight. */
        if (n >= 6 && !strcmp(tok[n - 1], "gold") &&
            parse_pos_int(tok[n - 2], &v) && !strcmp(tok[n - 3], "for")) {
            char nm[MAXNAME];
            if (!join_name(tok, 2, n - 3, nm)) return 0;
            cmd_fight_bounty(nm, v); return 1;
        }
        char nm[MAXNAME];
        if (!join_name(tok, 2, n, nm)) return 0;
        cmd_fight(nm); return 1;
    }

    if (!strcmp(verb, "heals")) {
        int v;
        if (n == 4 && parse_pos_int(tok[2], &v) && !strcmp(tok[3], "hp")) {
            cmd_heal(v); return 1;
        }
        return 0;
    }

    if (!strcmp(verb, "takes")) {
        int v;
        if (n == 4 && parse_pos_int(tok[2], &v) && !strcmp(tok[3], "damage")) {
            cmd_take(v); return 1;
        }
        return 0;
    }

    if (!strcmp(verb, "discards")) {
        if (n >= 4 && !strcmp(tok[2], "potion")) {
            char nm[MAXNAME];
            if (!join_name(tok, 3, n, nm)) return 0;
            cmd_discard(nm); return 1;
        }
        return 0;
    }

    if (!strcmp(verb, "sells")) {
        if (n < 7) return 0;
        if (strcmp(tok[n - 1], "gold")) return 0;
        int v;
        if (!parse_pos_int(tok[n - 2], &v)) return 0;
        if (strcmp(tok[n - 3], "for")) return 0;
        char nm[MAXNAME];
        if (n >= 8 && !strcmp(tok[2], "upgraded") && !strcmp(tok[3], "card")) {
            if (!join_name(tok, 4, n - 3, nm)) return 0;
            cmd_sell_upg(nm, v); return 1;
        }
        if (!strcmp(tok[2], "card")) {
            if (!join_name(tok, 3, n - 3, nm)) return 0;
            cmd_sell_card(nm, v); return 1;
        }
        if (!strcmp(tok[2], "potion")) {
            if (!join_name(tok, 3, n - 3, nm)) return 0;
            cmd_sell_potion(nm, v); return 1;
        }
        return 0;
    }

    if (!strcmp(verb, "marks")) {
        if (n < 6) return 0;
        if (strcmp(tok[2], "card")) return 0;
        if (strcmp(tok[n - 1], "exhaust")) return 0;
        if (strcmp(tok[n - 2], "as")) return 0;
        char nm[MAXNAME];
        if (!join_name(tok, 3, n - 2, nm)) return 0;
        cmd_mark_exhaust(nm); return 1;
    }

    return 0;
}

/* Main loop: read a line, dispatch it, print the result, repeat. */
int main(void) {
    char line[MAXLINE + 2];
    static char tok[MAXTOK][MAXNAME];
    int n;

    /* Make stdout flush after each line so the grader sees our output. */
    setvbuf(stdout, NULL, _IOLBF, 0);

    while (1) {
        if (!fgets(line, sizeof(line), stdin)) break;

        size_t L = strlen(line);
        if (L && line[L - 1] == '\n') line[L - 1] = '\0';

        /* Every response line starts with "» " as required. */
        printf("\xc2\xbb ");

        if (!tokenize(line, tok, &n)) { printf("INVALID\n"); continue; }
        if (n == 0) { printf("INVALID\n"); continue; }

        int r = dispatch(tok, n);
        if (r == 2) break;
        if (r == 0) printf("INVALID\n");
    }
    return 0;
}
