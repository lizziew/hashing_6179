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
            num_elements = 0;
            table = new bucket[num_buckets];
            for(int i = 0; i < num_buckets; i++) {
                table[i] = bucket();
            }
        }

        void print_table() {
            cout << "num_elements: " << num_elements << endl;
            cout << "num_buckets: " << num_buckets << endl;
            for(int i = 0; i < num_buckets; i++) {
                cout << "key: " << table[i].data.key << " val: " << table[i].data.value << " pl: " << table[i].probe_length << " flag: " << table[i].flag << endl;
            }
            cout << "............................................." << endl;
        }

        void insert(int key, int value) {
            if (num_elements + 1 >= LOAD_FACTOR_LIMIT * num_buckets) {
                double_table();
            }

            entry new_entry (key, value);
            bucket curr_bucket (new_entry, 0);
            int curr_index = find_slot(curr_bucket.data.key);

            //linear probe for an empty bucket in the table
            while (table[curr_index].flag != "EMPTY" && table[curr_index].flag != "DEL") {
                //move entry based on probe length
                if (curr_bucket.probe_length > table[curr_index].probe_length) {
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

        bool delete_entry(int key) {
            int curr_index = -1;

            for(int i = find_slot(key); i < num_buckets; i++) {
                if(table[i].data.key == key) {
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
                if(table[i].data.key == key) {
                    curr_index = i;
                    break;
                }
            }
            if (curr_index == -1) {
                //throw exception if we can't find key
                throw invalid_argument( "couldn't find value stored at key" );
            } else {
                return table[curr_index].data.value;
            }
        }
};

int main() {
    robin_hood_hash_table rh;

    //testing insert
    // rh.insert(1, 2);
    // rh.print_table();
    // rh.insert(3, 4);
    // rh.print_table();
    // rh.insert(5, 6);
    // rh.print_table();
    // rh.insert(7, 8);
    // rh.print_table();
    //
    // //testing retrieve
    // cout << rh.retrieve(1) << endl;
    // cout << rh.retrieve(3) << endl;
    // cout << rh.retrieve(5) << endl;
    // cout << rh.retrieve(7) << endl;
    //
    // //testing delete
    //
    // //// deleting nonexistent; expect false;
    // bool not_there = rh.delete_entry(111);
    // cout << not_there << endl;
    //
    // bool there = rh.delete_entry(7);
    // cout << there << endl;
    //
    // rh.print_table();

    for(int i = 0; i < 100; i++) {
      if(i % 2 == 0) {
        rh.insert(1,2);
      }
      else {
        rh.insert(3, 4);
      }
    }
    rh.print_table();
};
