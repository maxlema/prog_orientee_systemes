#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ERR_IO     -1
#define ERR_MALLOC -2
#define ERR_NULL   -3

#define MAX_CHARS 128

typedef struct
{
    char family[MAX_CHARS];
    char given[MAX_CHARS];
} name_t;

typedef struct
{
    unsigned short int country_code;
    unsigned short int area_code;
    long int number;
} phone_t;

typedef struct
{
    name_t name;
    phone_t phone;
} contact_t;

typedef struct
{
    contact_t* contacts;
    contact_t** indices;
    int NEnt;
} ypages_t;

int ct_compare(const contact_t* contact1, const contact_t* contact2)
{
    int r = strcmp(contact1->name.family, contact2->name.family);
    return (r == 0) ? strcmp((contact1->name.given), (contact2->name.given)) : r;
}

int is_prefix(const char *str1, const char *str2, unsigned int n)
{
    if(strlen(str1) < n || strlen(str2) < n) return 0;
    else return (strncmp(str1, str2, n) != 0) ? 0 : 1;
}

void ct_swap(const contact_t** contact1, const contact_t** contact2)
{
    contact_t* const tmp = *contact1;
    *contact1 = *contact2;
    *contact2 = tmp;
}

int print_contact(const contact_t* const contact, const FILE* file)
{
    if(contact == NULL) return ERR_NULL;
    if(fprintf(file, "%s \n", contact->name.family) < 0) return ERR_IO;
    if(fprintf(file, "%s \n", contact->name.given) < 0) return ERR_IO;
    if(fprintf(file, "%.4u %.4u %ld \n", contact->phone.country_code, contact->phone.area_code, contact->phone.number) < 0) return ERR_IO;
    return 0;
}

int print_ypages(const ypages_t* annuaire, const char* const file_name)
{
    FILE* file = fopen(file_name, "w"); //fopen(file_name, "r+");
    if (file == NULL) return ERR_IO;

    if(fprintf(file, "%d \n", annuaire->NEnt) < 0) return ERR_IO;

    for(int i=0; i<annuaire->NEnt; ++i) {
        if(print_contact(annuaire->indices[i], file)) return ERR_NULL;
    }
    fclose(file);
    return 0;
}

int read_ypages(ypages_t* const annuaire, const char* file_name) {
    FILE* file = fopen(file_name, "r");
    if (file == NULL) return ERR_IO;

    /*int nbr = 0;
    char nbrContacts[sizeof(int)];
    fgets(nbrContacts, sizeof(int), file);
    sscanf(nbrContacts, "%d", &nbr);*/

    //scan number of contacts
    int nbr = 0;
    if(fscanf(file, "%d", &nbr) < 0) return ERR_IO;

    //allocate the msize of memory we need
    if((annuaire->contacts = calloc(nbr, sizeof(contact_t))) == NULL) return ERR_MALLOC;
    if((annuaire->indices = calloc(nbr, sizeof(contact_t*))) == NULL) return ERR_MALLOC;
    annuaire->NEnt = nbr;

    //read the data and put it in ypages_t
    int counter = 0;
    while(!feof(file) && !ferror(file) && counter < nbr ) {
        if(fscanf(file, "%s %s %hu %hu %ld", annuaire->contacts[counter].name.family, annuaire->contacts[counter].name.given,
               &annuaire->contacts[counter].phone.country_code, &annuaire->contacts[counter].phone.area_code, &annuaire->contacts[counter].phone.number) < 0) return ERR_IO;

        annuaire->indices[counter] = &annuaire->contacts[counter];
        ++counter;
    }
    fclose(file);
    return 0;
}

int heapify(contact_t** tab, size_t i, size_t n) {
    //the algorithm as seen in algorithmic lecture
    size_t l = 2*i;
    size_t r = 2*i+1;
    size_t largest = (l<=n && 0 < ct_compare(tab[l-1], tab[i-1])) ? l : i;
    if(r<=n && 0 < ct_compare(tab[r-1], tab[largest-1])) largest = r;
	if(largest != i) {
        ct_swap(&tab[i-1], &tab[largest-1]);
        heapify(tab, largest, n);
	}
    return 0;
}


int buildheap(contact_t** tab, int n) {
    for(int i = n/2; i>=1; --i) {
        heapify(tab, i, n);
    }
    return 0;
}

void heapsort(ypages_t* annuaire) {
    //as seen in algorithmic lecture
    buildheap(annuaire->indices, annuaire->NEnt);
    for(int i = annuaire->NEnt; i>=2; --i) {
        ct_swap(&annuaire->indices[0], &annuaire->indices[i-1]);
        heapify(annuaire->indices, 1, i-1);
    }
}


unsigned int query(ypages_t* const annuaire, char* const qkey, FILE* file ) {
    //We first find the contact fitting the best our query
    if(annuaire==NULL) return ERR_NULL;
    int a = 0;
    int z = annuaire->NEnt-1;
    int middle = (a+z)/2;

    while(a < z) {
        int cmp = strcmp(annuaire->indices[middle]->name.family, qkey);
        if (cmp < 0) {
        a = middle+1;
        } else if (cmp == 0){
            a = middle;
            z = middle;
        } else {
        z = middle-1;
        }
        middle = (a+z)/2;
    }

    //Now we compute how many similar letters there are
    int mid = a;
    z = mid;
    unsigned int found = 0;
    int n = strlen(qkey);
    while(n != 0 && !is_prefix(annuaire->indices[a]->name.family, qkey, n)) {
        --n;
    }

    //if there is at least one similar letter, we check the upper and lower contacts in the sorted list
    //to see if they are at least as similar and we keep their bounds a and z
    if (n!=0) {
        ++found;
    while(a >= 0 && is_prefix(annuaire->indices[a]->name.family, qkey, n)) {
        if(is_prefix(annuaire->indices[a]->name.family, qkey, n+1)) {
            n = n+1;
            mid = a;
            z = mid;
        }
        if(a != mid) ++found;
            --a;
        }
        ++a;
        while(z < annuaire->NEnt && is_prefix(annuaire->indices[z]->name.family, qkey, n)) {
            if(z != mid) ++found;
            ++z;
        }
        --z;

        //Now we print all the contacts between the bounds that we found and return their number
        for(int i = a; i<=z; i++) {
            print_contact(annuaire->indices[i], file);
        }
    }
return found;
}

void free_ypages(ypages_t* annuaire) {
    free(annuaire->contacts);
    free(annuaire->indices);
    annuaire->contacts = NULL;
    annuaire->indices = NULL;
    annuaire->NEnt = 0;
}

int main()
{
    const char* const dat_fname     = "data.dat"     ;
    const char* const replica_fname = "data.replica" ;
    const char* const sorted_fname  = "data.sorted"  ;
    const char* const query_fname   = "query.dat"    ;
    const char* const result_fname  = "query.ans"    ;

    ypages_t yp = { NULL, NULL, 0 };
    int err = 0;

    /* Read the yellow pages register */
    err = read_ypages(&yp, dat_fname);
    if (err != 0) {

        fprintf(stderr, "Cannot read %s: %d\n", dat_fname, err);
        return ERR_IO;

    } else {

        /* Replicate the yellow pages register */
        err = print_ypages(&yp, replica_fname);
        if (err != 0) {
            fprintf(stderr, "Cannot write %s: %d\n", replica_fname, err);
            return ERR_IO;
        }

        /* Sort the yellow pages register */
        heapsort(&yp);

        /* Print out the sorted the yellow pages register */
        err = print_ypages(&yp, sorted_fname);
        if (err != 0) {
            fprintf(stderr, "Cannot write %s: %d\n", sorted_fname, err);
            return ERR_IO;
        }

        /* Open the query files */
        FILE* qin = fopen(query_fname, "r");
        if (NULL == qin) {
            fprintf(stderr, "[ERROR]: Cannot open 'query.dat' for reading (code %d)\n", ERR_IO);
            return ERR_IO;
        }

        FILE* qout = fopen(result_fname, "w");
        if (NULL == qout) {
            fprintf(stderr, "[ERROR]: Cannot open 'query.ans' for writing (code %d)\n", ERR_IO);
            fclose(qin);
            return ERR_IO;
        }

        /* Read the total number of queries */
        size_t nqueries = 0;
        err = fscanf(qin, "%zu", &nqueries);
        if (err != 1) {
            fprintf(stderr, "[ERROR]: Cannot read from 'query.dat' (code %d)\n", ERR_IO);
            fclose(qin);
            fclose(qout);
            return ERR_IO;
        }

        /* Iterate over all queries in 'query.dat' */
        for (size_t i = 0; i < nqueries; ++i) {

            /* Query key buffer */
            char qkey[MAX_CHARS];
            memset(qkey, 0, sizeof(qkey));

            /* Read one query key */
            err = fscanf(qin, " %s", qkey);
            if (err != 1) {
                fprintf(stderr, "[ERROR]: Cannot read from 'query.dat' (code %d)\n", ERR_IO);
                fclose(qin);
                fclose(qout);
                return ERR_IO;
            }

            /* Answer the query */
            unsigned int found = query(&yp, qkey, qout); // splited into two to ensure proper output order (query has side-effects on qout)
            fprintf(qout, "%u contacts found\n", found);
            fprintf(qout, "=====\n");
        }

        /* Close I/O files */
        fclose(qin);
        fclose(qout);
    }

    /* Free all allocated memory */
    free_ypages(&yp);

    /* Execution ended successfully */
    return 0;
}
