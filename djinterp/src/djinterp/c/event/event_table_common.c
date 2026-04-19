#include "../../../inc/c/event/event_table_common.h"


D_INLINE size_t
d_event_hash_function
(
    d_event_id _key,
    size_t     _table_size
)
{
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis
    uint64_t prime = 1099511628211ULL;       // FNV prime

    uint8_t* bytes = (uint8_t*)&_key;

    for (size_t i = 0; i < sizeof(d_event_id); i++)
    {
        hash ^= bytes[i];
        hash *= prime;
    }

    return hash % _table_size;
}

// Alternative simple hash (for comparison/testing)
D_INLINE size_t
d_event_hash_simple
(
    d_event_id _key,
    size_t     _table_size
)
{
    return (size_t)(_key ^ (_key >> 32)) % _table_size;
}

size_t 
d_event_hash_next_prime
(
    size_t _n
)
{
    if (_n <= 2)
    {
        return 2;
    }
    
    if (_n <= 3)
    {
        return 3;
    }
    
    // Start with next odd number
    if (_n % 2 == 0)
    {
        _n++;
    }
    
    while (1)
    {
        bool is_prime = true;
        
        for (size_t i = 3; i * i <= _n; i += 2)
        {
            if (_n % i == 0)
            {
                is_prime = false;
                break;
            }
        }
        
        if (is_prime)
        {
            return _n;
        }
        
        _n += 2;
    }
}

/******************************************************************************
* NODE MANAGEMENT
******************************************************************************/

struct d_event_hash_node* 
d_event_hash_node_new
(                            
    d_event_id               _key,
    struct d_event_listener* _value
)
{
    struct d_event_hash_node* node = malloc(sizeof(struct d_event_hash_node));
    
    if (!node)
    {
        return NULL;
    }
    
    node->key = _key;
    node->value = _value;
    node->next = NULL;
    
    return node;
}

void 
d_event_hash_node_free
(
    struct d_event_hash_node* _node
)
{
    if (_node)
    {
        // Note: We don't free the event_listener here as it's managed externally
        free(_node);
    }
}

/******************************************************************************
* STATISTICS AND DEBUGGING
******************************************************************************/

char*
d_event_hash_table_stats_to_string
(
    const struct d_event_hash_stats* _stats,
    const char*                      _label
)
{
    if (!_stats)
    {
        return NULL;
    }

    char buf[512];
    int len = snprintf(buf, sizeof(buf),
        "=== Hash Table Statistics: %s ===\n"
        "Total buckets:        %zu\n"
        "Used buckets:         %zu\n"
        "Total elements:       %zu\n"
        "Load factor:          %.3f\n"
        "Max chain length:     %zu\n"
        "Avg chain length:     %.3f\n"
        "Bucket utilization:   %.1f%%\n"
        "=====================================\n",
        _label 
            ? _label 
            : "Unknown",
        _stats->total_buckets,
        _stats->used_buckets,
        _stats->total_elements,
        _stats->load_factor,
        _stats->max_chain_length,
        _stats->average_chain_length,
        (_stats->total_buckets > 0)
            ? ( (double)(_stats->used_buckets) /
                (double)(_stats->total_buckets * 100.0) )
            : 0.0);

    if (len < 0)
    {
        return NULL;
    }

    char* result = malloc((size_t)len + 1);

    // ensure that memory allocation was successful
    if (!result)
    {
        return NULL;
    }

    memcpy(result, buf, (size_t)len + 1);

    return result;
}