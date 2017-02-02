#include <iostream>
#include <cassert>
#include <chrono>
#include <unordered_map>
#include <cmath>
#include <random>
#include <climits>

typedef std::chrono::high_resolution_clock Clock;

using namespace std;

#define INITIAL_CAPACITY 256
#define LOAD_FACTOR_LIMIT 0.9
#define TEST_COUNT 10000

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
      return key & (num_buckets - 1);
      key = (key ^ 61) ^ (key >> 16);
      key = key + (key << 3);
      key = key ^ (key>> 4);
      key = key * 0x27d4eb2d;
      key = key ^ (key >> 15);

      //quick way to take mod of power of 2
      return key & (num_buckets - 1);
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
      cout << "..................................." << endl;
    }

    void insert(pair<int, int> data) {
      insert(get<0>(data), get<1>(data));
    }

    void insert(int key, int value) {
      if (num_elements + 1 >= LOAD_FACTOR_LIMIT * num_buckets) {
        double_table();
      }

      entry new_entry (key, value);
      bucket curr_bucket (new_entry, 0);
      int curr_index = find_slot(curr_bucket.data.key);

      //linear probe for an empty bucket in the table
      while (table[curr_index].flag != "EMPTY" && table[curr_index].flag != "DEL" && curr_index < num_buckets) {
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

    int contains(int key) {
      int curr_index = -1;

      for (int i = find_slot(key); i < num_buckets; i++) {
        if (table[i].data.key == key) {
          curr_index = i;
          break;
        }
      }
      
      return curr_index == -1;
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

int geometric_sum(int base, int magnitude) {
    return (1 - pow(base, magnitude + 1)) / (1 - base);
}

/**
 * Benchmark the robin hood hashtable against std::unordered_map. For each magnitude i, base^i operations
 * are performed, logging results for each magnitude. The expected proportion of operations that 
 * are insertions is prob_insert, and the expected proportion of operations that are deletions 
 * is prob_delete. The rest of the operations are contains. All queried keys and values are integers.
 * The operation types are randomly ordered and decided beforehand for both hashtable implementations.
*/
void benchmark(int base, int magnitude, float prob_insert, float prob_delete) {
  robin_hood_hash_table rh;
  unordered_map<int, int> um;

  int num_ops = geometric_sum(base, magnitude);
  int op_types[num_ops];
  int keys[num_ops];
  int values[num_ops];

  const int OP_CONTAINS = 0;
  const int OP_INSERT = 1;
  const int OP_DELETE = 2;

  random_device op_rd;
  mt19937 op_rng(op_rd());
  float prob_contains = 1.0f - prob_insert - prob_delete;
  discrete_distribution<int> op_dist {prob_contains, prob_insert, prob_delete};

  random_device int_rd;
  mt19937 int_rng(int_rd());
  uniform_int_distribution<int> int_dist(INT_MIN, INT_MAX);

  for (int i = 0; i < num_ops; i++) {
    op_types[i] = op_dist(op_rng);
    keys[i] = int_dist(int_rng);
    values[i] = int_dist(int_rng);
  }

  cout << "Robin hood" << endl;
  cout << "num_operations,throughput" << endl;

  auto t1 = Clock::now();
  auto t2 = Clock::now();
  for (int i = 0; i < magnitude; i++) {
    t1 = Clock::now();
    int last = geometric_sum(base, i + 1);

    for (int j = geometric_sum(base, i); j < last; j++) {
      switch(op_types[j]) {
        case OP_CONTAINS:
          rh.contains(keys[j]);
          break;

        case OP_INSERT:
          rh.insert(keys[j], values[j]);
          break;

        case OP_DELETE:
          rh.delete_entry(keys[j]);
          break;
      }
    }

    t2 = Clock::now();
    int ops_round = pow(base, i);
    long duration = chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count();
    long throughput = ops_round / (duration / pow(10,9));
    cout << ops_round << ',' << throughput << endl;
  }

  cout << "std::unordered_map" << endl;
  cout << "num_operations,throughput" << endl;

  t1 = Clock::now();
  for (int i = 0; i < magnitude; i++) {
    t1 = Clock::now();
    int last = geometric_sum(base, i + 1);

    for (int j = geometric_sum(base, i); j < last; j++) {
      switch(op_types[j]) {
        case OP_CONTAINS:
          um.find(keys[j]);
          break;

        case OP_INSERT:
          um[keys[j]] = values[j];
          break;

        case OP_DELETE:
          um.erase(keys[j]);
          break;
      }
    }

    t2 = Clock::now();
    int ops_round = pow(base, i);
    long duration = chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count();
    long throughput = ops_round / (duration / pow(10,9));
    cout << ops_round << ',' << throughput << endl;
  }
}

int main() {
  benchmark(2, 16, 0.1f, 0.1f);

  /*
    robin_hood_hash_table rh;
    unordered_map<int, int> m;
  
    //testing sorted insert
    cout << "TESTING ON SORTED INPUT FROM [0, 100K)" << endl;
    auto t1 = Clock::now();
    for(int i = 0; i < TEST_COUNT; i++) {
      rh.insert(i, i+1);
    }
    auto t2 = Clock::now();
    cout << "rh: "
                << chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count()
                << " nanoseconds" << endl;
  
    t1 = Clock::now();
    for(int i = 0; i < TEST_COUNT; i++) {
      m.insert(make_pair(i, i+1));
    }
    t2 = Clock::now();
    cout << "std: "
                << chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count()
                << " nanoseconds" << endl;
  
    //testing rand input
    cout << "TESTING ON UNSORTED INPUT FROM [0, 100K)" << endl;
    t1 = Clock::now();
    for(int i = 0; i < TEST_COUNT; i++) {
      rh.insert(rand() % TEST_COUNT, rand() % TEST_COUNT);
    }
    t2 = Clock::now();
    cout << "rh: "
                << chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count()
                << " nanoseconds" << endl;
  
    t1 = Clock::now();
    for(int i = 0; i < TEST_COUNT; i++) {
      m.insert(make_pair(rand() % TEST_COUNT, rand() % TEST_COUNT));
    }
    t2 = Clock::now();
    cout << "std: "
                << chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count()
                << " nanoseconds" << endl;
  
    //testing retrieve
    // time(&timer);
    // cout << rh.retrieve(1) << endl;
    // seconds = difftime(timer,mktime(&y2k));
    // cout << seconds << endl;
    //
    // time(&timer);
    // cout << rh.retrieve(3) << endl;
    // seconds = difftime(timer,mktime(&y2k));
    // cout << seconds << endl;
    //
    // time(&timer);
    // cout << rh.retrieve(5) << endl;
    // seconds = difftime(timer,mktime(&y2k));
    // cout << seconds << endl;
    //
    // time(&timer);
    // cout << rh.retrieve(7) << endl;
    // seconds = difftime(timer,mktime(&y2k));
    // cout << seconds << endl;
  
  
    //testing delete
  
    //// deleting nonexistent; expect false;
    // time(&timer);
    // bool not_there = rh.delete_entry(111);
    // seconds = difftime(timer,mktime(&y2k));
    // cout << seconds << endl;
    // assert(!not_there);
    //
    // time(&timer);
    // bool there = rh.delete_entry(7);
    // seconds = difftime(timer,mktime(&y2k));
    // cout << seconds << endl;
    // assert(there);
    */
}
