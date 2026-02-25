#include "../../../inc/c/event/event_table.h"


/******************************************************************************
* CREATION AND DESTRUCTION
******************************************************************************/

struct d_event_hash_table* 
d_event_hash_table_new
(
    size_t _initial_size
)
{
    size_t prime_size;
    struct d_event_hash_table* new_table;

    if (_initial_size == 0)
    {
        _initial_size = D_EVENT_HASH_TABLE_DEFAULT_SIZE;
    }
    
    new_table = malloc(sizeof(struct d_event_hash_table));
    
    if (!new_table)
    {
        return NULL;
    }
    
    prime_size = d_event_hash_next_prime(_initial_size);
    new_table->buckets = calloc(prime_size, sizeof(struct d_event_hash_node*));
    
    if (!new_table->buckets)
    {
        free(new_table);
        return NULL;
    }
    
    new_table->size = prime_size;
    new_table->count = 0;
    new_table->enabled_count = 0;
    
    return new_table;
}

struct d_event_hash_table* 
d_event_hash_table_new_default
(
    void
)
{
    return d_event_hash_table_new(D_EVENT_HASH_TABLE_DEFAULT_SIZE);
}

void 
d_event_hash_table_free
(
    struct d_event_hash_table* _table
)
{
    if (!_table)
    {
        return;
    }
    
    d_event_hash_table_clear(_table);
    free(_table->buckets);
    free(_table);
}

/******************************************************************************
* CORE OPERATIONS
******************************************************************************/

bool 
d_event_hash_table_insert
(
    struct d_event_hash_table* _table,
    d_event_id                 _key,
    struct d_event_listener*   _listener
)
{
    bool was_enabled;
    size_t index;
    struct d_event_hash_node* current;
    struct d_event_hash_node* new_node;

    if (!_table || !_listener)
    {
        return false;
    }
    
    index = d_event_hash_function(_key, _table->size);
    
    // Check if key already exists
    current = _table->buckets[index];
    
    while (current)
    {
        if (current->key == _key)
        {
            // Update existing entry
            was_enabled = current->value->enabled;
            current->value = _listener;
            
            // Update enabled count
            if (was_enabled && !_listener->enabled)
            {
                _table->enabled_count--;
            }
            else if (!was_enabled && _listener->enabled)
            {
                _table->enabled_count++;
            }
            
            return true;
        }
        
        current = current->next;
    }
    
    // Create new node
    new_node = d_event_hash_node_new(_key, _listener);
    
    if (!new_node)
    {
        return false;
    }
    
    // Insert at head of chain
    new_node->next = _table->buckets[index];
    _table->buckets[index] = new_node;
    _table->count++;
    
    if (_listener->enabled)
    {
        _table->enabled_count++;
    }
    
    // Check if resize is needed
    if (d_event_hash_table_load_factor(_table) > D_EVENT_HASH_TABLE_LOAD_FACTOR)
    {
        d_event_hash_table_resize(_table, _table->size * D_EVENT_HASH_TABLE_RESIZE_FACTOR);
    }
    
    return true;
}

struct d_event_listener* 
d_event_hash_table_lookup
(
    const struct d_event_hash_table* _table,
    d_event_id                       _key
)
{
    size_t index;
    struct d_event_hash_node* current;

    if (!_table)
    {
        return NULL;
    }
    
    index = d_event_hash_function(_key, _table->size);
    current = _table->buckets[index];
    
    while (current)
    {
        if (current->key == _key)
        {
            return current->value;
        }
        
        current = current->next;
    }
    
    return NULL;
}

bool 
d_event_hash_table_remove
(
    struct d_event_hash_table* _table,
    d_event_id                 _key
)
{
    size_t index;
    struct d_event_hash_node* current;
    struct d_event_hash_node* prev;

    if (!_table)
    {
        return false;
    }
    
    index = d_event_hash_function(_key, _table->size);
    current = _table->buckets[index];
    prev = NULL;
    
    while (current)
    {
        if (current->key == _key)
        {
            // Remove node from chain
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                _table->buckets[index] = current->next;
            }
            
            if (current->value->enabled)
            {
                _table->enabled_count--;
            }
            
            d_event_hash_node_free(current);
            _table->count--;
            
            return true;
        }
        
        prev = current;
        current = current->next;
    }
    
    return false;
}

bool 
d_event_hash_table_contains
(
    const struct d_event_hash_table* _table,
    d_event_id                       _key
)
{
    return d_event_hash_table_lookup(_table, _key) != NULL;
}

/******************************************************************************
* TABLE MANAGEMENT
******************************************************************************/

size_t 
d_event_hash_table_size
(
    const struct d_event_hash_table* _table
)
{
    return _table ? _table->size : 0;
}

size_t 
d_event_hash_table_count
(
    const struct d_event_hash_table* _table
)
{
    return _table ? _table->count : 0;
}

size_t 
d_event_hash_table_enabled_count
(
    const struct d_event_hash_table* _table
)
{
    return _table ? _table->enabled_count : 0;
}

double 
d_event_hash_table_load_factor
(
    const struct d_event_hash_table* _table
)
{
    if (!_table || _table->size == 0)
    {
        return 0.0;
    }
    
    return (double)_table->count / (double)_table->size;
}

bool 
d_event_hash_table_resize
(
    struct d_event_hash_table* _table,
    size_t                     _new_size
)
{
    size_t prime_new_size;
    struct d_event_hash_node** new_buckets;
    struct d_event_hash_node** old_buckets;
    size_t old_size;

    if (!_table)
    {
        return false;
    }
    
    prime_new_size = d_event_hash_next_prime(_new_size);
    new_buckets = calloc(prime_new_size, sizeof(struct d_event_hash_node*));
    
    if (!new_buckets)
    {
        return false;
    }
    
    // Rehash all existing nodes
    old_buckets = _table->buckets;
    old_size = _table->size;
    
    _table->buckets = new_buckets;
    _table->size = prime_new_size;
    
    for (size_t i = 0; i < old_size; i++)
    {
        struct d_event_hash_node* current = old_buckets[i];
        
        while (current)
        {
            struct d_event_hash_node* next = current->next;
            
            // Rehash and insert into new table
            size_t new_index = d_event_hash_function(current->key, prime_new_size);
            current->next = new_buckets[new_index];
            new_buckets[new_index] = current;
            
            current = next;
        }
    }
    
    free(old_buckets);
    return true;
}

void 
d_event_hash_table_clear
(
    struct d_event_hash_table* _table
)
{
    if (!_table)
    {
        return;
    }
    
    for (size_t i = 0; i < _table->size; i++)
    {
        struct d_event_hash_node* current = _table->buckets[i];
        
        while (current)
        {
            struct d_event_hash_node* next = current->next;
            d_event_hash_node_free(current);
            current = next;
        }
        
        _table->buckets[i] = NULL;
    }
    
    _table->count = 0;
    _table->enabled_count = 0;
}

/******************************************************************************
* ITERATOR IMPLEMENTATION
******************************************************************************/

struct d_event_hash_iterator*
d_event_hash_table_iterator_begin
(
    const struct d_event_hash_table* _table
)
{
    struct d_event_hash_iterator* iter;

    if (!_table)
    {
        return NULL;
    }

    iter = malloc(sizeof(struct d_event_hash_iterator));
    
    if (!iter)
    {
        return NULL;
    }

    iter->table = _table;
    iter->bucket_index = 0;
    iter->current_node = NULL;
    
    // Find first non-empty bucket
    for (size_t i = 0; i < _table->size; i++)
    {
        if (_table->buckets[i])
        {
            iter->bucket_index = i;
            iter->current_node = _table->buckets[i];
            break;
        }
    }
    
    return iter;
}

bool 
d_event_hash_table_iterator_has_next
(
    struct d_event_hash_iterator* _iter
)
{
    return _iter && _iter->table && _iter->current_node;
}

struct d_event_listener* 
d_event_hash_table_iterator_next
(
    struct d_event_hash_iterator* _iter,
    d_event_id*                   _key_out
)
{
    struct d_event_listener* result;

    if (!d_event_hash_table_iterator_has_next(_iter))
    {
        return NULL;
    }
    
    result = _iter->current_node->value;
    
    if (_key_out)
    {
        *_key_out = _iter->current_node->key;
    }
    
    // Advance iterator
    _iter->current_node = _iter->current_node->next;
    
    // If current chain is exhausted, find next non-empty bucket
    if (!_iter->current_node)
    {
        _iter->bucket_index++;
        
        while (_iter->bucket_index < _iter->table->size)
        {
            if (_iter->table->buckets[_iter->bucket_index])
            {
                _iter->current_node = _iter->table->buckets[_iter->bucket_index];
                break;
            }
            
            _iter->bucket_index++;
        }
    }
    
    return result;
}

void 
d_event_hash_table_iterator_free
(
    struct d_event_hash_iterator* _iter
)
{
    if (_iter)
    {
        free(_iter);
    }
}

/******************************************************************************
* STATISTICS
******************************************************************************/

struct d_event_hash_stats 
d_event_hash_table_get_stats
(
    const struct d_event_hash_table* _table
)
{
    struct d_event_hash_stats stats = {0};
    size_t chain_lengths;
    
    if (!_table)
    {
        return stats;
    }
    
    stats.total_buckets = _table->size;
    stats.total_elements = _table->count;
    stats.load_factor = d_event_hash_table_load_factor(_table);
    
    chain_lengths = 0;
    
    for (size_t i = 0; i < _table->size; i++)
    {
        if (_table->buckets[i])
        {
            size_t chain_length = 0;
            struct d_event_hash_node* current = _table->buckets[i];
            
            stats.used_buckets++;
            
            while (current)
            {
                chain_length++;
                current = current->next;
            }
            
            chain_lengths += chain_length;
            
            if (chain_length > stats.max_chain_length)
            {
                stats.max_chain_length = chain_length;
            }
        }
    }
    
    stats.average_chain_length = stats.used_buckets > 0 ? 
        (double)chain_lengths / (double)stats.used_buckets : 0.0;
    
    return stats;
}