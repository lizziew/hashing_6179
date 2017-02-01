#include <iostream>
using namespace std;

#define INITIAL_CAPACITY 256
#define LOAD_FACTOR_LIMIT 0.9

typedef struct entry {
    int key;
    int value;

    entry() {
        key = 0;
        value = 0;
    }

    entry(int k, int v) : key(k), value(v) {}
} entry;

typedef struct bucket {
    entry data;
    int probe_length;
    string flag; //"DEL" "EMPTY" "FULL"

    bucket() {
        data = entry();
        probe_length = 0;
        flag = "EMPTY";
    }

    bucket(entry e, int pl) : data(e), probe_length(pl), flag("FULL") {}
} bucket;

class robin_hood_hash_table {
    private:
        bucket* table;
        int num_buckets;
        int num_elements;

        int find_slot(int key) {
            //quick way to take mod of power of 2
            return hash<int>()(key) & (num_buckets - 1);
        }

        void double_table() {
            int original_num_buckets = num_buckets;
            num_buckets *= 2;

            bucket *old_table = table;
            table = new bucket[num_buckets];

            num_elements = 0;

            for(int i = 0; i < original_num_buckets; i++) {
                if(table[i].flag != "DEL") {
                    table[i] = old_table[i];
                    num_elements++;
                }
            }

            delete [] old_table;
        }

        void swap_buckets(bucket* a, bucket* b) {
            bucket t = *a;
            *a = *b;
            *b = t;
        }

    public:
        robin_hood_hash_table() {
            num_buckets = INITIAL_CAPACITY;
            table = new bucket[num_buckets];
            for(int i = 0; i < num_buckets; i++) {
                table[i] = bucket();
            }
        }

        void insert(int key, int value) {
            if (num_elements + 1 >= LOAD_FACTOR_LIMIT * num_buckets) {
                double_table();
            }

            entry new_entry (key, value);
            bucket curr_bucket (new_entry, 0);
            int curr_index = find_slot(curr_bucket.data.key);

            //linear probe for an empty bucket in the table
            while (table[curr_index].flag != "EMPTY" || table[curr_index].flag != "DEL"){
                //move entry based on probe length
                if (curr_bucket.probe_length > table[curr_index].probe_length){
                    swap_buckets(&curr_bucket, &table[curr_index]);
                }

                //increment index for linear probing
                curr_index += 1;
                curr_bucket.probe_length += 1;
            }

            //replace bucket at table[curr_index] with curr_bucket
            table[curr_index] = curr_bucket;
            num_elements += 1;
        }

        bool delete(int key) {
            int curr_index = -1;

            for(int i = find_slot(key); i < num_buckets; i++) {
                if(table[i].entry.key == key) {
                    curr_index = i;
                    table[i].flag = "DEL";
                    num_elements -= 1;
                    break;
                }
            }

            return curr_index != -1;
        }

        int retrieve(int key) {
            int curr_index = -1;

            for(int i = find_slot(key); i < num_buckets; i++) {
                if(table[i].entry.key == key) {
                    curr_index = i;
                    break;
                }
            }
            if (curr_index == -1) {
                //throw exception if we can't find key
                throw invalid_argument( "couldn't find value stored at key" );
            } else {
                return table[curr_index].entry.value;
            }
        }
};
