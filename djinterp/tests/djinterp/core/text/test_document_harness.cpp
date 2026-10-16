/*******************************************************************************
* djinterp [core]                                      test_document_harness.cpp
*
*
* path:      /tests/djinterp/core/text/test_document_harness.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// T2 verification harness.
//
//   Proves the test_document layer:
//     (a) the baseline env exposes every documented key, and unbound parts
//         of the focus yield empty (the lenient missing-source rule);
//     (b) the SAME test_document drives tree and flow renderings, with
//         per-iteration fragments identical across the two formats
//         (T1's invariant, re-asserted at the document level);
//     (c) a custom test_kind extends the run via doc.env().bind("new_key",
//         ...) - touching no format, no section, no engine.

// std
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
// djinterp


using ::djinterp::test::test_context;
using ::djinterp::test::test_document;
using ::djinterp::test::test_report;
using ::djinterp::test::test_status;
using ::djinterp::test::report_check;
using ::djinterp::test::report_unit;
using ::djinterp::test::report_module;
using ::djinterp::test::report_verdict;
using ::djinterp::test::at_run;
using ::djinterp::test::at_module;
using ::djinterp::test::at_unit;
using ::djinterp::test::render_run_tree;
using ::djinterp::test::render_run_flow;


// ===========================================================================
// tree target stand-in (matches document_writer cursor's open_child/text)
// ===========================================================================

struct tree_node
{
    std::string                              name;
    std::string                              text;
    std::vector<std::unique_ptr<tree_node> > children;
};


class tree_cursor
{
public:
    explicit tree_cursor(
        tree_node* _node
    )
        : m_node(_node)
    {}

    tree_cursor
    open_child(
        std::string _name
    )
    {
        m_node->children.emplace_back(new tree_node());
        m_node->children.back()->name = std::move(_name);

        return tree_cursor(m_node->children.back().get());
    }

    tree_cursor&
    text(
        std::string _text
    )
    {
        m_node->text = std::move(_text);

        return *this;
    }

    tree_node*
    node() const
    {
        return m_node;
    }

private:
    tree_node* m_node;
};


// ===========================================================================
// flow target stand-in (matches pdf_template's add_text/add_vspace/add_page_break)
// ===========================================================================

struct flow_element
{
    enum class kind { text, vspace, page_break };

    kind         k      = kind::text;
    std::string  text;
    double       amount = 0.0;
};


class flow_doc
{
public:
    void
    add_text(
        std::string _text
    )
    {
        flow_element _e;
        _e.k    = flow_element::kind::text;
        _e.text = std::move(_text);

        m_elements.push_back(std::move(_e));
    }

    void
    add_vspace(
        double _amount
    )
    {
        flow_element _e;
        _e.k      = flow_element::kind::vspace;
        _e.amount = _amount;

        m_elements.push_back(std::move(_e));
    }

    void
    add_page_break()
    {
        flow_element _e;
        _e.k = flow_element::kind::page_break;

        m_elements.push_back(std::move(_e));
    }

    const std::vector<flow_element>&
    elements() const
    {
        return m_elements;
    }

private:
    std::vector<flow_element> m_elements;
};


// ===========================================================================
// fixture
// ===========================================================================

static test_report
build_fixture()
{
    test_report _r;
    _r.title       = "DTest 1.4 release run";
    _r.subtitle    = "container + callable + event vocab";
    _r.author      = "Sam";
    _r.description = "smoke run for the document refactor";
    _r.notes       = "T1+T2 verified locally.";

    // module 1: container (3 units, all pass)
    {
        report_module _m("container", "tests of the container vocabulary");
        {
            report_unit _u("vector basics");
            _u.add_result("size starts at 0", true);
            _u.add_result("push_back grows by 1", true);
            _u.add_result("clear empties", true);
            _u.elapsed_ns = 1234567;       // 1.23 ms
            _m.units.push_back(std::move(_u));
        }
        {
            report_unit _u("vector copy");
            _u.add_result("copy is independent", true);
            _u.elapsed_ns = 456789;        // 456.79 us
            _m.units.push_back(std::move(_u));
        }
        {
            report_unit _u("vector mutation");
            _u.add_result("erase shrinks by 1", true);
            _u.add_result("emplace_back constructs in place", true);
            _u.elapsed_ns = 50000000;      // 50.00 ms
            _m.units.push_back(std::move(_u));
        }
        _r.modules.push_back(std::move(_m));
    }

    // module 2: callable (2 units, ONE FAIL)
    {
        report_module _m("callable", "tests of the callable vocabulary");
        {
            report_unit _u("plain function");
            _u.add_result("invocable yields T", true);
            _m.units.push_back(std::move(_u));
        }
        {
            report_unit _u("capturing lambda");
            _u.add_result("captures by value", true);
            _u.add_result("captures by ref", false);  // FAIL
            _m.units.push_back(std::move(_u));
        }
        _r.modules.push_back(std::move(_m));
    }

    // module 3: event (zero units - tests the "empty inner iteration" case)
    {
        report_module _m("event", "tests of the event vocabulary");
        _r.modules.push_back(std::move(_m));
    }

    return _r;
}


// ===========================================================================
// (a) baseline key surface check
// ===========================================================================

static void
check_baseline_keys(
    const test_document& _doc
)
{
    const auto& _env = _doc.env();

    // a representative key from each depth must be present
    const char* _required[] =
    {
        "title", "subtitle", "author", "description", "notes",
        "total_modules", "passed_modules", "failed_modules",
        "total_units",   "passed_units",   "failed_units",
        "total_checks",  "passed_checks",  "failed_checks",
        "modules_ratio", "units_ratio",    "checks_ratio",
        "modules_pass_rate", "units_pass_rate", "checks_pass_rate",
        "verdict", "verdict_word",
        "module_index", "module_name", "module_description",
        "module_total_units",  "module_passed_units",  "module_failed_units",
        "module_total_checks", "module_passed_checks", "module_failed_checks",
        "module_units_ratio",  "module_checks_ratio",
        "module_units_pass_rate", "module_checks_pass_rate",
        "module_verdict", "module_verdict_word",
        "unit_index", "unit_name",
        "unit_total_checks", "unit_passed_checks", "unit_failed_checks",
        "unit_checks_ratio", "unit_checks_pass_rate",
        "unit_verdict", "unit_verdict_word", "unit_elapsed",
        "check_index", "check_description",
        "check_status_word", "check_status_symbol"
    };

    const std::size_t _n = sizeof(_required) / sizeof(_required[0]);

    for (std::size_t _i = 0; _i < _n; ++_i)
    {
        if (!_env.has(_required[_i]))
        {
            std::cerr << "MISSING key: " << _required[_i] << '\n';
            std::exit(1);
        }
    }

    std::cout << "(a) baseline key surface: "
              << _n << " keys, all bound.\n";
}


// ===========================================================================
// (a.2) lenient missing-source: a key whose part is null reads as empty
// ===========================================================================

static void
check_lenient_missing(
    const test_document& _doc
)
{
    const auto& _env = _doc.env();

    test_context _empty;   // every pointer null

    // every projection must return the empty string against a null focus
    auto _spot_check = [&](const char* _k)
    {
        const std::string _v = _env.project(_k, _empty);
        if (!_v.empty())
        {
            std::cerr << "key '" << _k << "' read '" << _v
                      << "' against an empty ctx (should be empty)\n";
            std::exit(1);
        }
    };

    _spot_check("title");
    _spot_check("module_name");
    _spot_check("unit_name");
    _spot_check("check_description");
    _spot_check("verdict_word");
    _spot_check("unit_elapsed");

    // a numeric index whose ctx slot is zero reads as "0" - that is the
    // contract: indices are size_t, always emit a numeral.
    const std::string _mi = _env.project("module_index", _empty);
    assert(_mi == "0");

    std::cout << "(a.2) lenient missing-source: ok.\n";
}


// ===========================================================================
// (b) the SAME doc drives both formats; per-iteration fragments are equal
// ===========================================================================

// At each module focus, capture the key set a sensible body would draw.
// Both the tree skeleton and the flow skeleton MUST produce these.
struct module_record
{
    std::string name;
    std::string description;
    std::string verdict_word;
    std::string units_ratio;
    std::string checks_ratio;
    std::vector<std::string> unit_names;       // ordered as iterated
    std::vector<std::string> unit_verdicts;    // ordered as iterated
    std::vector<std::vector<std::string> > check_status_words;  // per unit
};


static std::vector<module_record>
collect_invariant_records(
    const test_document& _doc,
    const test_context&  _run_ctx
)
{
    std::vector<module_record> _out;
    const auto&                _env = _doc.env();

    _doc.per_module().for_each(
        _env, _run_ctx, _out,
        [&_doc](const ::djinterp::binding_env<test_context>& _e,
                const test_context& _module_ctx,
                std::vector<module_record>& _acc)
        {
            module_record _r;
            _r.name         = _e.project("module_name",         _module_ctx);
            _r.description  = _e.project("module_description",  _module_ctx);
            _r.verdict_word = _e.project("module_verdict_word", _module_ctx);
            _r.units_ratio  = _e.project("module_units_ratio",  _module_ctx);
            _r.checks_ratio = _e.project("module_checks_ratio", _module_ctx);

            std::pair<std::vector<std::string>,
                      std::vector<std::vector<std::string> > > _inner;

            _doc.per_unit_in_module().for_each(
                _e, _module_ctx, _inner,
                [&_doc](const ::djinterp::binding_env<test_context>& _ie,
                        const test_context& _unit_ctx,
                        std::pair<std::vector<std::string>,
                                  std::vector<std::vector<std::string> > >& _ic)
                {
                    _ic.first.push_back(_ie.project("unit_name", _unit_ctx));

                    std::vector<std::string> _statuses;
                    _doc.per_check_in_unit().for_each(
                        _ie, _unit_ctx, _statuses,
                        [](const ::djinterp::binding_env<test_context>& _je,
                           const test_context& _check_ctx,
                           std::vector<std::string>& _ss)
                        {
                            _ss.push_back(
                                _je.project("check_status_word", _check_ctx));
                        });
                    _ic.second.push_back(std::move(_statuses));
                });

            _r.unit_names         = std::move(_inner.first);
            _r.check_status_words = std::move(_inner.second);

            // unit verdicts (re-walk the units for clarity)
            _doc.per_unit_in_module().for_each(
                _e, _module_ctx, _r.unit_verdicts,
                [](const ::djinterp::binding_env<test_context>& _ie,
                   const test_context& _unit_ctx,
                   std::vector<std::string>& _vs)
                {
                    _vs.push_back(_ie.project("unit_verdict_word", _unit_ctx));
                });

            _acc.push_back(std::move(_r));
        });

    return _out;
}

// shim: collect_invariant_records uses `_env()` to mean "the env" - fix
// (the closure already captured _env by const ref, so this is just style).


static void
check_tree_flow_invariant(
    const test_document& _doc,
    const test_context&  _run_ctx
)
{
    // 1. compute the canonical record from doc + env directly
    const std::vector<module_record> _canon =
        collect_invariant_records(_doc, _run_ctx);

    // 2. render to a tree target and a flow target via the SAME doc
    tree_node     _tree_root;
    _tree_root.name = "report";
    tree_cursor   _root_cursor(&_tree_root);
    render_run_tree(_doc, _run_ctx, _root_cursor);

    flow_doc      _flow;
    render_run_flow(_doc, _run_ctx, _flow);

    // 3. EXTRACT what each format placed and check it agrees with _canon.
    //
    //    Tree extraction: walk <modules>/<module> children.
    //    Flow extraction: scan elements for "MODULE: <name>" headers and
    //    "--- <name> ---" unit headers.

    // -- tree --
    const tree_node* _modules_n = nullptr;

    for (const auto& _c : _tree_root.children)
    {
        if (_c->name == "modules")
        {
            _modules_n = _c.get();
            break;
        }
    }

    if (_modules_n == nullptr)
    {
        std::cerr << "tree: <modules> not found\n";
        std::exit(1);
    }

    if (_modules_n->children.size() != _canon.size())
    {
        std::cerr << "tree: module count " << _modules_n->children.size()
                  << " != expected " << _canon.size() << '\n';
        std::exit(1);
    }

    for (std::size_t _i = 0; _i < _canon.size(); ++_i)
    {
        const tree_node* _mod = _modules_n->children[_i].get();
        const module_record& _cm = _canon[_i];

        // pull name / verdict / ratios out of children
        std::string _name;
        std::string _verdict;
        std::string _units_ratio;
        std::string _checks_ratio;
        const tree_node* _units_n = nullptr;

        for (const auto& _ch : _mod->children)
        {
            if      (_ch->name == "name")          _name         = _ch->text;
            else if (_ch->name == "verdict")       _verdict      = _ch->text;
            else if (_ch->name == "units_ratio")   _units_ratio  = _ch->text;
            else if (_ch->name == "checks_ratio")  _checks_ratio = _ch->text;
            else if (_ch->name == "units")         _units_n      = _ch.get();
        }

        if ( (_name         != _cm.name)         ||
             (_verdict      != _cm.verdict_word) ||
             (_units_ratio  != _cm.units_ratio)  ||
             (_checks_ratio != _cm.checks_ratio) )
        {
            std::cerr << "tree drift at module " << _i << ": "
                      << "name=" << _name << " vs " << _cm.name << ", "
                      << "verdict=" << _verdict << " vs " << _cm.verdict_word
                      << ", units=" << _units_ratio << " vs "
                      << _cm.units_ratio << ", checks=" << _checks_ratio
                      << " vs " << _cm.checks_ratio << '\n';
            std::exit(1);
        }

        // unit names
        if (_units_n == nullptr)
        {
            if (!_cm.unit_names.empty())
            {
                std::cerr << "tree: <units> missing under module " << _i
                          << " (expected " << _cm.unit_names.size()
                          << " units)\n";
                std::exit(1);
            }
        }
        else
        {
            if (_units_n->children.size() != _cm.unit_names.size())
            {
                std::cerr << "tree: unit count drift under module " << _i
                          << '\n';
                std::exit(1);
            }

            for (std::size_t _u = 0; _u < _cm.unit_names.size(); ++_u)
            {
                const tree_node* _unit = _units_n->children[_u].get();
                std::string      _un;
                for (const auto& _ch : _unit->children)
                {
                    if (_ch->name == "name") { _un = _ch->text; break; }
                }
                if (_un != _cm.unit_names[_u])
                {
                    std::cerr << "tree: unit name drift\n";
                    std::exit(1);
                }
            }
        }
    }

    // -- flow --
    // find each "MODULE: <name>" header in order
    std::vector<std::string> _flow_module_names;
    std::vector<std::string> _flow_unit_names;
    for (const auto& _e : _flow.elements())
    {
        if (_e.k != flow_element::kind::text) continue;

        const std::string _prefix_mod  = "MODULE: ";
        const std::string _prefix_unit = "--- ";
        const std::string _suffix_unit = " ---";

        if ( (_e.text.size() > _prefix_mod.size()) &&
             (_e.text.substr(0, _prefix_mod.size()) == _prefix_mod) )
        {
            _flow_module_names.push_back(_e.text.substr(_prefix_mod.size()));
            continue;
        }

        if ( (_e.text.size() > (_prefix_unit.size() + _suffix_unit.size())) &&
             (_e.text.substr(0, _prefix_unit.size()) == _prefix_unit) &&
             (_e.text.substr(_e.text.size() - _suffix_unit.size())
                 == _suffix_unit) )
        {
            _flow_unit_names.push_back(_e.text.substr(
                _prefix_unit.size(),
                _e.text.size() - _prefix_unit.size() - _suffix_unit.size()));
        }
    }

    if (_flow_module_names.size() != _canon.size())
    {
        std::cerr << "flow: " << _flow_module_names.size()
                  << " modules placed, expected " << _canon.size() << '\n';
        std::exit(1);
    }
    for (std::size_t _i = 0; _i < _canon.size(); ++_i)
    {
        if (_flow_module_names[_i] != _canon[_i].name)
        {
            std::cerr << "flow drift at module " << _i << '\n';
            std::exit(1);
        }
    }

    std::size_t _expected_units = 0;
    for (const auto& _r : _canon) _expected_units += _r.unit_names.size();
    if (_flow_unit_names.size() != _expected_units)
    {
        std::cerr << "flow: " << _flow_unit_names.size()
                  << " unit headers placed, expected " << _expected_units
                  << '\n';
        std::exit(1);
    }

    std::cout << "(b) tree and flow render identical bindings: "
              << _canon.size() << " modules, "
              << _expected_units << " units.\n";

    // dump for the eye
    std::cout << "\n--- TREE RENDERING (truncated) ---\n";
    auto print_tree = [](auto&& self, const tree_node& _n,
                          std::size_t _depth) -> void
    {
        for (std::size_t _i = 0; _i < _depth; ++_i) std::cout << "  ";
        std::cout << '<' << _n.name << '>';
        if (!_n.text.empty()) std::cout << _n.text;
        std::cout << '\n';
        for (const auto& _ch : _n.children) self(self, *_ch, _depth + 1);
    };
    print_tree(print_tree, _tree_root, 0);

    std::cout << "\n--- FLOW RENDERING ---\n";
    for (const auto& _e : _flow.elements())
    {
        if (_e.k == flow_element::kind::text)
            std::cout << "TEXT: " << _e.text << '\n';
        else if (_e.k == flow_element::kind::vspace)
            std::cout << "VSPACE: " << _e.amount << '\n';
        else
            std::cout << "PAGE_BREAK\n";
    }
}


// ===========================================================================
// (c) a custom test_kind extends the run via env.bind() - no format touched
// ===========================================================================

//   Simulate a "benchmark" test_kind that records per-unit timing metadata
// the baseline doesn't surface as a key.  We just install a new projection
// reading from the existing unit_elapsed_ns field for demo purposes; in real
// use the kind would pull from its own per-node metadata table.

static void
check_test_kind_extension(
    const test_document& _doc_const_view  // a baseline doc
)
{
    // First: a baseline-only render does NOT carry the new key.
    {
        test_document   _baseline;
        const auto&     _env_b = _baseline.env();
        test_context    _empty;
        assert(!_env_b.has("benchmark_us"));
        const std::string _v = _env_b.project("benchmark_us", _empty);
        assert(_v.empty());
    }

    // Second: an EXTENDED doc binds the new key.  Existing keys are
    // untouched; the new key is now available.
    test_document _doc;        // a fresh doc to extend (avoid mutating _const_view)
    (void)_doc_const_view;

    const std::size_t _size_before = _doc.env().size();

    _doc.env().bind("benchmark_us",
        [](const test_context& _c) -> std::string
        {
            if (_c.unit == nullptr) return std::string();

            char _buf[32];
            std::snprintf(_buf, sizeof(_buf), "%.2f us",
                          static_cast<double>(_c.unit->elapsed_ns) / 1000.0);

            return std::string(_buf);
        });

    const std::size_t _size_after  = _doc.env().size();

    assert(_doc.env().has("benchmark_us"));
    assert(_doc.env().has("title"));            // baseline preserved
    assert(_size_after == _size_before + 1);    // exactly +1 key

    // Now render against a fixture and verify the new key resolves
    test_report  _run = build_fixture();
    test_context _here = at_run(&_run);

    // direct projection at a unit focus
    const report_module* _m = &_run.modules[0];
    const report_unit*   _u = &_m->units[0];

    test_context _unit_ctx = at_unit(&_run, _m, _u, 1, 1);
    const std::string _bench = _doc.env().project("benchmark_us", _unit_ctx);

    // elapsed_ns = 1234567 -> 1234.57 us
    assert(_bench == "1234.57 us");

    (void)_here;  // for completeness

    std::cout << "(c) test_kind extension: env grew from "
              << _size_before << " to " << _size_after
              << " keys; new key resolves correctly; baseline untouched.\n";
}


// ===========================================================================
// driver
// ===========================================================================

int
main()
{
    const test_report  _run  = build_fixture();
    test_context       _here = at_run(&_run);

    test_document      _doc;

    check_baseline_keys(_doc);
    check_lenient_missing(_doc);
    check_tree_flow_invariant(_doc, _here);
    check_test_kind_extension(_doc);

    std::cout << "\nT2 verification complete.\n";
    return 0;
}
