/*******************************************************************************
* djinterp [cpp]                                               mongodb_table.hpp
*
* A collection of a MongoDB database held in memory: its documents, read,
* changed in place, and written back.
*   mongo_table binds one collection, or view, of the database a
* mongo_connection reaches to a cache of its documents. refresh() reads them
* -- every one, or those a filter, a sort and a page select -- each beside
* its key, {"_id": ...}, the filter that finds the document it was read as
* (d_mongodb_find_keyed); the cache is then read and changed document by
* document, and commit() writes back what changed: a document the cache no
* longer holds is deleted, a changed one replaced, by its key, and a new one
* inserted; no other document is touched. Documents are canonical Extended
* JSON, as read and as written, so that every value keeps its type: an
* int64 stays an int64, a date a date.
*   commit() writes as one unit, in a transaction of its own, where the
* deployment has transactions (a replica set, a sharded cluster, a load
* balancer); inside one the program began, as part of it, which MongoDB has
* no savepoint to spare: a failure there leaves the program's transaction to
* the program, failed on the server where a write itself failed. A
* standalone server has no transactions, and a commit there is refused
* unless set_atomic(false) lets it write without one, in order, a failure
* keeping the writes before it.
*   The table is MongoDB's own, not database_table, whose cache and writes
* are SQL's rows and statements: a document is not a row of columns, and
* its fields' types are BSON's, which the generic value cannot carry back
* (an int32 and an int64 are one integer there). The table's members keep
* database_table's names where they mean the same -- refresh, commit,
* is_dirty, exists, create_table, drop_table, rename_table, truncate_table,
* row_count_remote -- and its other settings, with no meaning here, it has
* not (db_audit.md, 12).
*
*
* path:      /inc/djinterp/cpp/db/mongodb/mongodb_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.23
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DOCUMENTS
    ---------
    1.  The cache's documents
         1.  mongo_document
2.  MONGODB TABLE
    -------------
    1.  The class
         1.  mongo_table
*/

#ifndef DJINTERP_DB_MONGODB_MONGODB_TABLE_HPP
#define DJINTERP_DB_MONGODB_MONGODB_TABLE_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>    // std::size_t
#include <map>        // std::map
#include <memory>     // std::unique_ptr
#include <optional>   // std::optional, std::nullopt
#include <set>        // std::set
#include <stdexcept>  // std::out_of_range
#include <string>     // std::string, std::to_string
#include <utility>    // std::move, std::exchange
#include <vector>     // std::vector
// djinterp
#include "../../djinterp.hpp"    // framework root
#include "../database.hpp"          // query_exception, transaction
#include "../database_statement.hpp" // buffered_result_set
#include "./mongodb.hpp"            // mongo_connection
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::int64_t


NS_DJINTERP


//==============================================================================
// 1.  DOCUMENTS
//==============================================================================


// 1.1    The cache's documents
//------------------------------------------------------------------------------
// 1.1.1
// mongo_document
//   struct: one document of a mongo_table's cache: its key, {"_id": ...},
// the filter that finds the document of the database it was read as, or
// std::nullopt for one the cache never read, which a commit inserts; and the
// document, as canonical Extended JSON. A document's own _id must stay the
// key's, which a replacement cannot change.
struct mongo_document
{
    std::optional<std::string> key;
    std::string                document;
};


//==============================================================================
// 2.  MONGODB TABLE
//==============================================================================


// 2.1    The class
//------------------------------------------------------------------------------
// 2.1.1
// mongo_table
//   class template: a collection of the database a mongo_connection reaches,
// its current one, and a cache of its documents, as the banner describes.
// The table holds a pointer to the connection, which must outlive it, and
// every exchange with the database goes through it. Config is carried for
// what builds on the table (config_type), and read by nothing here.
//   Element access is checked: a position past the end throws
// std::out_of_range. Every member that reaches the database throws as the
// connection's operations do, and connection_exception when the table has
// no open connection. Not copyable; movable, the table moved from bound to
// no connection and holding nothing.
//
// Usage:
//     mongo_connection   db("mongodb://db.example/shop?replicaSet=rs0");
//     mongo_table<>      orders(db, "orders");
//
//     db.connect();
//     orders.set_filter(R"({"status": "open"})");
//     orders.refresh();                    // the open orders
//     orders.set_document(0, changed);     // canonical Extended JSON
//     orders.remove_document(1);
//     orders.commit();                     // a replace and a delete, as one
template<typename Config = void>
class mongo_table
{
public:
    // identity and container -- the table, its connection, and its
    // documents
    using self_type       = mongo_table<Config>;
    using config_type     = Config;
    using connection_type = mongo_connection;
    using value_type      = mongo_document;
    using size_type       = std::size_t;
    using storage_type    = std::vector<mongo_document>;
    using const_iterator  = typename storage_type::const_iterator;

    // construction -- bound to no connection; or to a connection and a
    // collection's name. Nothing is read: refresh() reads the documents
    mongo_table()
        : m_connection(nullptr),
          m_atomic(true),
          m_dirty(false),
          m_stale(true),
          m_keyless(false)
    {}

    mongo_table(
        mongo_connection& _connection,
        std::string       _collection
    )
        : m_connection(&_connection),
          m_collection(std::move(_collection)),
          m_atomic(true),
          m_dirty(false),
          m_stale(true),
          m_keyless(false)
    {}

    mongo_table(const mongo_table&)            = delete;
    mongo_table& operator=(const mongo_table&) = delete;

    mongo_table(
        mongo_table&& _other
    ) noexcept
        : m_connection(std::exchange(_other.m_connection, nullptr)),
          m_collection(std::move(_other.m_collection)),
          m_filter(std::move(_other.m_filter)),
          m_sort(std::move(_other.m_sort)),
          m_limit(_other.m_limit),
          m_offset(_other.m_offset),
          m_atomic(_other.m_atomic),
          m_dirty(_other.m_dirty),
          m_stale(_other.m_stale),
          m_keyless(_other.m_keyless),
          m_documents(std::move(_other.m_documents)),
          m_read(std::move(_other.m_read))
    {
        _other.m_reset_moved();
    }

    mongo_table&
    operator=(
        mongo_table&& _other
    ) noexcept
    {
        // the other's state, and the other left bound to nothing
        if (this != &_other)
        {
            m_connection = std::exchange(_other.m_connection, nullptr);
            m_collection = std::move(_other.m_collection);
            m_filter     = std::move(_other.m_filter);
            m_sort       = std::move(_other.m_sort);
            m_limit      = _other.m_limit;
            m_offset     = _other.m_offset;
            m_atomic     = _other.m_atomic;
            m_dirty      = _other.m_dirty;
            m_stale      = _other.m_stale;
            m_keyless    = _other.m_keyless;
            m_documents  = std::move(_other.m_documents);
            m_read       = std::move(_other.m_read);
            _other.m_reset_moved();
        }

        return *this;
    }

    ~mongo_table() = default;

    // capacity -- the documents the cache holds
    size_type
    size() const noexcept
    {
        return m_documents.size();
    }

    bool
    empty() const noexcept
    {
        return m_documents.empty();
    }

    // access -- a document of the cache, its key, and its text, by position;
    // std::out_of_range past the end
    const mongo_document&
    at(
        size_type _index
    ) const
    {
        m_check_index(_index, "at");

        return m_documents[_index];
    }

    const mongo_document&
    operator[](
        size_type _index
    ) const
    {
        return at(_index);
    }

    const std::optional<std::string>&
    key(
        size_type _index
    ) const
    {
        return at(_index).key;
    }

    const std::string&
    document(
        size_type _index
    ) const
    {
        return at(_index).document;
    }

    // iteration -- over the cache's documents, which change through the
    // members below alone
    const_iterator
    begin() const noexcept
    {
        return m_documents.cbegin();
    }

    const_iterator
    end() const noexcept
    {
        return m_documents.cend();
    }

    // changing the cache -- a document replaced, its key kept; a document
    // added, with no key, which commit() inserts, its index returned; one
    // removed, which commit() deletes where it was read; and every one
    // removed. std::out_of_range for a position past the end; nothing
    // reaches the database before commit()
    void
    set_document(
        size_type   _index,
        std::string _document
    )
    {
        m_check_index(_index, "set_document");
        m_documents[_index].document = std::move(_document);
        m_dirty                      = true;

        return;
    }

    size_type
    add_document(
        std::string _document
    )
    {
        m_documents.push_back({ std::nullopt, std::move(_document) });
        m_dirty = true;

        return (m_documents.size() - 1u);
    }

    void
    remove_document(
        size_type _index
    )
    {
        m_check_index(_index, "remove_document");
        m_documents.erase(m_documents.begin() +
                          static_cast<std::ptrdiff_t>(_index));
        m_dirty = true;

        return;
    }

    void
    clear() noexcept
    {
        m_dirty = ( (m_dirty) ||
                    (!m_documents.empty()) );
        m_documents.clear();

        return;
    }

    // connection -- the connection the table is bound to, nullptr for none;
    // binding another forgets the documents read, the cache keeping its
    // documents as ones the new connection's collection has not seen
    mongo_connection*
    get_connection() noexcept
    {
        return m_connection;
    }

    const mongo_connection*
    get_connection() const noexcept
    {
        return m_connection;
    }

    void
    set_connection(
        mongo_connection* _connection
    )
    {
        m_connection = _connection;
        m_forget_read();

        return;
    }

    // is_connected -- whether the table is bound to a connection that is open
    bool
    is_connected() const noexcept
    {
        return ( (m_connection) &&
                 (m_connection->is_connected()) );
    }

    // identity -- the collection's name, in the connection's current
    // database; setting another forgets the documents read, as
    // set_connection does
    const std::string&
    table_name() const noexcept
    {
        return m_collection;
    }

    void
    set_table_name(
        std::string _collection
    )
    {
        m_collection = std::move(_collection);
        m_forget_read();

        return;
    }

    // query -- what refresh() reads: the filter, a JSON document ("" for
    // every document); the sort, a JSON document ({"qty": -1}); and a page,
    // its size and the documents before it skipped; each makes the cache
    // stale
    void
    set_filter(
        std::string _filter
    )
    {
        m_filter = std::move(_filter);
        m_stale  = true;

        return;
    }

    const std::string&
    get_filter() const noexcept
    {
        return m_filter;
    }

    void
    set_sort(
        std::string _sort
    )
    {
        m_sort  = std::move(_sort);
        m_stale = true;

        return;
    }

    const std::string&
    get_sort() const noexcept
    {
        return m_sort;
    }

    void
    set_limit(
        std::optional<size_type> _limit
    )
    {
        m_limit = _limit;
        m_stale = true;

        return;
    }

    std::optional<size_type>
    get_limit() const noexcept
    {
        return m_limit;
    }

    void
    set_offset(
        std::optional<size_type> _offset
    )
    {
        m_offset = _offset;
        m_stale  = true;

        return;
    }

    std::optional<size_type>
    get_offset() const noexcept
    {
        return m_offset;
    }

    // atomic -- whether commit() must write as one unit, and so refuses on a
    // deployment without transactions; true unless set false
    void
    set_atomic(
        bool _atomic
    ) noexcept
    {
        m_atomic = _atomic;

        return;
    }

    bool
    is_atomic() const noexcept
    {
        return m_atomic;
    }

    // state -- whether the cache was changed since it was read or written;
    // and whether it may be behind its query: never read, or its query
    // changed since
    bool
    is_dirty() const noexcept
    {
        return m_dirty;
    }

    bool
    is_stale() const noexcept
    {
        return m_stale;
    }

    /**
     * @brief Reads the documents the query selects into the cache, replacing
     *        what it held, each beside its key.
     *
     * @throws connection_exception when the table has no open connection;
     *         query_exception, with the layer's result, when the find fails
     *         (a filter or a sort in error).
     * @post On success the cache holds the documents, is neither dirty nor
     *       stale, and is what commit() compares with; on failure it is as
     *       it was.
     */
    void
    refresh()
    {
        validate_connected("refresh");

        storage_type                       documents;
        std::map<std::string, std::string> read;
        bool                               keyless = false;

        // a page of none is none, which MongoDB's limit of 0 is not
        if ( (!m_limit.has_value()) ||
             (*m_limit > 0u) )
        {
            std::unique_ptr<buffered_result_set> found =
                m_connection->find_keyed(m_collection,
                                         m_filter,
                                         m_find_options());

            m_take(*found, documents, read, keyless);
        }

        m_documents = std::move(documents);
        m_read      = std::move(read);
        m_keyless   = keyless;
        m_dirty     = false;
        m_stale     = false;

        return;
    }

    /**
     * @brief Writes the cache's changes to the collection, as the banner
     *        describes: the documents read and no longer held deleted, the
     *        changed ones replaced by their keys, the new ones inserted, in
     *        that order, as one bulk write.
     *
     * @throws connection_exception when the table has no open connection;
     *         query_exception with D_DB_UNSUPPORTED for documents read
     *         without an _id (a view's projection), and, on a deployment
     *         without transactions, unless set_atomic(false); with
     *         D_DB_NOT_FOUND for a changed document another session deleted;
     *         and with the server's code for a write it refused (11000 for a
     *         duplicate key, 66 for a replacement that changes the _id), the
     *         unit rolled back; transaction_exception when the unit cannot
     *         begin or end. The refusals but the last two come before
     *         anything is written.
     * @post On success the cache is not dirty, and is what the next commit()
     *       compares with; one that inserted documents is read again, to hold
     *       the _id each was given. On failure the database is as it was,
     *       save as the banner says, and the cache as it was.
     */
    void
    commit()
    {
        validate_connected("commit");

        std::string     operations;
        re_std::int64_t replaced = 0;
        re_std::int64_t inserted = 0;

        m_plan(operations, replaced, inserted);

        // nothing to write
        if (operations.empty())
        {
            m_dirty = false;

            return;
        }

        m_write(operations, replaced);

        // the documents as the database now holds them
        if (inserted > 0)
        {
            refresh();
        }
        else
        {
            m_settle();
        }

        return;
    }

    /**
     * @brief Says whether the database has the collection, or a view of the
     *        name.
     *
     * @return whether it is there.
     * @throws connection_exception when the table has no open connection;
     *         query_exception, with the layer's result, when the database
     *         cannot tell.
     */
    bool
    exists() const
    {
        validate_connected("exists");

        return ( (m_connection->has_collection(m_collection)) ||
                 (m_connection->has_view(m_collection)) );
    }

    /**
     * @brief Counts the documents the filter selects in the database,
     *        without reading them.
     *
     * @return the count; 0 for a collection that is not there.
     * @throws as exists().
     */
    re_std::int64_t
    row_count_remote() const
    {
        validate_connected("row_count_remote");

        return m_connection->count_documents(m_collection, m_filter);
    }

    // the collection itself -- creating it, with the create command's
    // options, one there a success with _if_not_exists; dropping it, its
    // indexes with it, the cache then holding nothing read; renaming it,
    // one there under the new name dropped first with _drop_target; and
    // deleting every document, its indexes kept, the cache emptied. Each
    // throws as exists(), and query_exception with the server's code when
    // it refuses
    void
    create_table(
        bool               _if_not_exists = true,
        const std::string& _options       = ""
    )
    {
        validate_connected("create_table");
        m_connection->create_collection(m_collection,
                                        _options,
                                        _if_not_exists);

        return;
    }

    void
    drop_table(
        bool _if_exists = true
    )
    {
        validate_connected("drop_table");
        m_connection->drop_collection(m_collection, _if_exists);
        m_forget_read();
        m_documents.clear();
        m_dirty = false;

        return;
    }

    void
    rename_table(
        const std::string& _new_name,
        bool               _drop_target = false
    )
    {
        validate_connected("rename_table");
        m_connection->rename_collection(m_collection,
                                        _new_name,
                                        _drop_target);
        m_collection = _new_name;

        return;
    }

    void
    truncate_table()
    {
        validate_connected("truncate_table");
        (void)m_connection->delete_many(m_collection, "");
        m_documents.clear();
        m_read.clear();
        m_keyless = false;
        m_dirty   = false;

        return;
    }

private:
    // validate_connected -- connection_exception unless the table is bound
    // to an open connection
    void
    validate_connected(
        const char* _operation
    ) const
    {
        // a connection, and an open one
        if (!is_connected())
        {
            throw connection_exception(std::string("mongo_table::") +
                                       _operation + ": no open connection");
        }

        return;
    }

    // m_check_index -- std::out_of_range for a position past the end
    void
    m_check_index(
        size_type   _index,
        const char* _operation
    ) const
    {
        // a document there
        if (_index >= m_documents.size())
        {
            throw std::out_of_range(std::string("mongo_table::") +
                                    _operation + ": no document " +
                                    std::to_string(_index));
        }

        return;
    }

    // m_find_options -- refresh()'s options: the sort and the page
    std::string
    m_find_options() const
    {
        std::string options;

        // each member given, a comma between
        if (!m_sort.empty())
        {
            options += "\"sort\": " + m_sort;
        }

        if (m_offset.has_value())
        {
            options += (options.empty()) ? "" : ", ";
            options += "\"skip\": " + std::to_string(*m_offset);
        }

        if (m_limit.has_value())
        {
            options += (options.empty()) ? "" : ", ";
            options += "\"limit\": " + std::to_string(*m_limit);
        }

        return (options.empty()) ? options
                                 : "{" + options + "}";
    }

    // m_take -- the documents a keyed find read, and each by its key
    static void
    m_take(
        buffered_result_set&                _found,
        storage_type&                       _documents,
        std::map<std::string, std::string>& _read,
        bool&                               _keyless
    )
    {
        // each row: the key, or none, and the document
        while (_found.next())
        {
            mongo_document read_one;

            read_one.key      = _found.get_string(static_cast<std::size_t>(0));
            read_one.document = _found.get_string(static_cast<std::size_t>(1))
                                    .value_or(std::string());

            if (read_one.key.has_value())
            {
                _read[*read_one.key] = read_one.document;
            }
            else
            {
                _keyless = true;
            }

            _documents.push_back(std::move(read_one));
        }

        return;
    }

    /**
     * @brief Plans a commit: the writes, as a bulk write's models, in the
     *        order they go -- deletes, replacements, inserts -- and how many
     *        replace and insert.
     *
     * @param[out] _operations  the models, a JSON array; "" for none.
     * @param[out] _replaced    the replacements.
     * @param[out] _inserted    the inserts.
     * @throws query_exception with D_DB_UNSUPPORTED for documents read
     *         without an _id.
     */
    void
    m_plan(
        std::string&     _operations,
        re_std::int64_t& _replaced,
        re_std::int64_t& _inserted
    ) const
    {
        std::set<std::string> held;
        std::string           replaces;
        std::string           inserts;

        // documents that cannot be written back
        if (m_keyless)
        {
            throw query_exception("mongo_table::commit: documents read without "
                                  "an _id cannot be written back",
                                  D_DB_UNSUPPORTED);
        }

        // each document held: unchanged, changed, or new; a key is held
        // once, as refresh() read it
        for (const mongo_document& held_one : m_documents)
        {
            const auto read_one = (held_one.key.has_value())
                                      ? m_read.find(*held_one.key)
                                      : m_read.end();

            if (held_one.key.has_value())
            {
                held.insert(*held_one.key);
            }

            if (read_one == m_read.end())
            {
                m_add_model(inserts,
                            "{\"insertOne\": {\"document\": " +
                                held_one.document + "}}");
                ++_inserted;
            }
            else if (read_one->second != held_one.document)
            {
                m_add_model(replaces,
                            "{\"replaceOne\": {\"filter\": " +
                                *held_one.key + ", \"replacement\": " +
                                held_one.document + "}}");
                ++_replaced;
            }
        }

        // the documents read and no longer held, first
        for (const auto& read_one : m_read)
        {
            if (held.find(read_one.first) == held.end())
            {
                m_add_model(_operations,
                            "{\"deleteOne\": {\"filter\": " + read_one.first +
                                "}}");
            }
        }

        m_add_model(_operations, replaces);
        m_add_model(_operations, inserts);

        if (!_operations.empty())
        {
            _operations = "[" + _operations + "]";
        }

        return;
    }

    // m_add_model -- a model, or models, after those before, a comma between
    static void
    m_add_model(
        std::string&       _models,
        const std::string& _model
    )
    {
        // nothing to add
        if (_model.empty())
        {
            return;
        }

        _models += (_models.empty()) ? "" : ", ";
        _models += _model;

        return;
    }

    /**
     * @brief Writes the models as one unit where the deployment can, as the
     *        banner says.
     *
     * @param[in] _operations  the models.
     * @param[in] _replaced    the replacements among them, each of which must
     *                         match its document.
     * @throws as commit().
     */
    void
    m_write(
        const std::string& _operations,
        re_std::int64_t    _replaced
    )
    {
        // inside the program's transaction, as part of it
        if (m_connection->in_transaction())
        {
            m_send(_operations, _replaced);

            return;
        }

        // without transactions, only where the table allows it
        if (!m_connection->supports_transactions())
        {
            if (m_atomic)
            {
                throw query_exception("mongo_table::commit: the deployment "
                                      "has no transactions, which a commit "
                                      "needs to write as one unit; "
                                      "set_atomic(false) writes without one",
                                      D_DB_UNSUPPORTED);
            }

            m_send(_operations, _replaced);

            return;
        }

        transaction<mongo_connection> unit(*m_connection);

        m_send(_operations, _replaced);
        unit.commit();

        return;
    }

    // m_send -- the bulk write, ordered, and D_DB_NOT_FOUND where a
    // replacement found no document, another session having deleted it
    void
    m_send(
        const std::string& _operations,
        re_std::int64_t    _replaced
    )
    {
        const mongo_write_result written =
            m_connection->bulk_write(m_collection, _operations, true);

        // a replacement that matched nothing
        if (written.matched < _replaced)
        {
            throw query_exception("mongo_table::commit: " +
                                      std::to_string(_replaced -
                                                     written.matched) +
                                      " changed document(s) are no longer "
                                      "in the collection",
                                  D_DB_NOT_FOUND);
        }

        return;
    }

    // m_settle -- the cache, written, as the documents read
    void
    m_settle()
    {
        std::map<std::string, std::string> read;

        // each document by its key
        for (const mongo_document& held_one : m_documents)
        {
            if (held_one.key.has_value())
            {
                read[*held_one.key] = held_one.document;
            }
        }

        m_read  = std::move(read);
        m_dirty = false;

        return;
    }

    // m_forget_read -- the documents held no longer stand for any read: a
    // commit inserts each
    void
    m_forget_read() noexcept
    {
        for (mongo_document& held_one : m_documents)
        {
            held_one.key.reset();
        }

        m_read.clear();
        m_keyless = false;
        m_stale   = true;
        m_dirty   = ( (m_dirty) ||
                      (!m_documents.empty()) );

        return;
    }

    // m_reset_moved -- the table moved from: bound to nothing, holding
    // nothing
    void
    m_reset_moved() noexcept
    {
        m_connection = nullptr;
        m_collection.clear();
        m_filter.clear();
        m_sort.clear();
        m_limit.reset();
        m_offset.reset();
        m_documents.clear();
        m_read.clear();
        m_atomic  = true;
        m_dirty   = false;
        m_stale   = true;
        m_keyless = false;

        return;
    }

    mongo_connection*                  m_connection;
    std::string                        m_collection;
    std::string                        m_filter;
    std::string                        m_sort;
    std::optional<size_type>           m_limit;
    std::optional<size_type>           m_offset;
    bool                               m_atomic;
    bool                               m_dirty;
    bool                               m_stale;
    bool                               m_keyless;  // a document read had no
                                                   // _id
    storage_type                       m_documents;
    std::map<std::string, std::string> m_read;     // key: document, as read
};


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MONGODB_MONGODB_TABLE_HPP
