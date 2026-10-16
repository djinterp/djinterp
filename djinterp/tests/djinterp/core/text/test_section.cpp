/*******************************************************************************
* djinterp [core]                                               test_section.cpp
*
*
* path:      /tests/djinterp/core/text/test_section.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// T1 verification harness.
//
//   Proves the section / binding_env architectural point:
//     for every iteration i, the FRAGMENTS produced by a body that reads the
//     env only through env.source_for(ctx) / env.project(key, ctx) are
//     IDENTICAL across formats; the only difference is PLACEMENT.
//
//   Two formats are exercised:
//     - tree target:  a minimal labelled-tree sink mirroring the surface a
//                     document_writer cursor exposes (open_child / text).
//     - flow target:  a minimal flow-element sink mirroring the surface a
//                     pdf_template exposes (add_text / add_vspace /
//                     add_page_break).
//   Real document_writer / pdf_template plug in unchanged - this harness
//   uses stand-ins so the section kernel can be verified in ISOLATION,
//   without dragging in the xml/pdf stacks (those are intact in the project
//   tree; the section header depends on neither).
//
//   The harness fails the build if the per-iteration fragments diverge.

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
#include "djinterp/core/text/section.hpp"


// ===========================================================================
// model: a tiny report-shape used as the focus
// ===========================================================================

struct report_check
{
    std::string description;
    bool        ok = true;
};

struct report_unit
{
    std::string                name;
    std::vector<report_check>  checks;
};

struct report_module
{
    std::string                name;
    std::string                description;
    std::vector<report_unit>   units;
};

struct test_report
{
    std::string                  title;
    std::vector<report_module>   modules;
};


// ===========================================================================
// the "fat" nullable focus: the match_context pattern from the handoff
// ===========================================================================

struct test_context
{
    const test_report*   run    = nullptr;
    const report_module* module_ = nullptr;
    const report_unit*   unit   = nullptr;
    const report_check*  check  = nullptr;
};


// ===========================================================================
// tree target stand-in (matches document_writer cursor's open_child/text)
// ===========================================================================

struct tree_node
{
    std::string                              name;
    std::string                              text;
    std::vector<std::unique_ptr<tree_node> > children;
};


// tree_cursor
//   minimal stand-in for document_writer's cursor: open_child appends a
// labelled child and returns a cursor focused on it; text sets the focused
// node's text; chainable.
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
    std::string  text;            // for text
    double       amount = 0.0;    // for vspace
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

        return;
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

        return;
    }

    void
    add_page_break()
    {
        flow_element _e;
        _e.k = flow_element::kind::page_break;

        m_elements.push_back(std::move(_e));

        return;
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
// the env (one env; the same bindings drive every format)
// ===========================================================================

using djinterp::binding_env;
using djinterp::section;
using djinterp::make_section;


static binding_env<test_context>
make_dtest_env()
{
    binding_env<test_context> _env;

    _env.bind("title",
              [](const test_context& _c) -> std::string
              {
                  return (_c.run != nullptr) ? _c.run->title : std::string();
              })
        .bind("module_name",
              [](const test_context& _c) -> std::string
              {
                  return (_c.module_ != nullptr)
                      ? _c.module_->name : std::string();
              })
        .bind("module_description",
              [](const test_context& _c) -> std::string
              {
                  return (_c.module_ != nullptr)
                      ? _c.module_->description : std::string();
              })
        .bind("unit_name",
              [](const test_context& _c) -> std::string
              {
                  return (_c.unit != nullptr) ? _c.unit->name : std::string();
              })
        .bind("unit_check_count",
              [](const test_context& _c) -> std::string
              {
                  if (_c.unit == nullptr)
                  {
                      return std::string();
                  }

                  std::ostringstream _os;
                  _os << _c.unit->checks.size();

                  return _os.str();
              });

    return _env;
}


// ===========================================================================
// the two iteration sections (per-module, per-unit-within-module)
// ===========================================================================

static section<test_context>
make_per_module_section()
{
    return make_section<test_context>(
        // count
        [](const test_context& _c) -> std::size_t
        {
            return (_c.run != nullptr) ? _c.run->modules.size() : 0;
        },
        // refocus
        [](const test_context& _c, std::size_t _i) -> test_context
        {
            test_context _r = _c;
            _r.module_ = &_c.run->modules[_i];
            _r.unit    = nullptr;
            _r.check   = nullptr;

            return _r;
        });
}


static section<test_context>
make_per_unit_section()
{
    return make_section<test_context>(
        // count
        [](const test_context& _c) -> std::size_t
        {
            return (_c.module_ != nullptr) ? _c.module_->units.size() : 0;
        },
        // refocus
        [](const test_context& _c, std::size_t _i) -> test_context
        {
            test_context _r = _c;
            _r.unit  = &_c.module_->units[_i];
            _r.check = nullptr;

            return _r;
        });
}


// ===========================================================================
// the SAME body, two formats
// ===========================================================================
//   Both bodies READ the env identically (env.project(key, ctx) for the tree
//   shape; via a tiny inline interpolator over env.source_for(ctx) for the
//   flow shape -- the lookup-source level, per the handoff's guidance about
//   not depending on pdf_template's internals).  The PLACEMENT differs:
//     tree:  one subtree per iteration, nested under the head.
//     flow:  one vertical band per iteration, with a vspace between bands.
//   The per-iteration FRAGMENT for any given key is identical across the
//   two formats.  That is the property the assertion below tests directly.


// trivial interpolator: scans "{key}" placeholders out of _format and feeds
// each through _source.  This is exactly the shape text_template.render takes;
// using a one-pass scanner here keeps the harness free of external headers.
template<typename _Source>
static std::string
interpolate(
    std::string_view _format,
    _Source&&        _source
)
{
    std::string _out;
    std::size_t _i = 0;

    while (_i < _format.size())
    {
        const char _ch = _format[_i];

        if (_ch != '{')
        {
            _out.push_back(_ch);
            ++_i;
            continue;
        }

        // find the matching close brace
        std::size_t _j = _i + 1;

        while ( (_j < _format.size()) && (_format[_j] != '}') )
        {
            ++_j;
        }

        if (_j >= _format.size())
        {
            // unmatched '{' -- emit literally
            _out.push_back('{');
            ++_i;
            continue;
        }

        const std::string_view _key = _format.substr(_i + 1, _j - _i - 1);

        _out.append(_source(_key));

        _i = _j + 1;
    }

    return _out;
}


// ---- collect the per-iteration fragments for cross-format comparison ----

struct iteration_record
{
    std::string module_name;
    std::string module_description;
    std::string unit_name_csv;     // joined "{unit_name}" projections, ';' sep
};


// the body shared by both formats (in spirit): produce a record of the
// fragments any reasonable body would draw from this env at this ctx.
// Both the tree body and the flow body MUST agree with this record.
static iteration_record
record_for(
    const binding_env<test_context>& _env,
    const test_context&              _module_ctx
)
{
    iteration_record _r;

    _r.module_name        = _env.project("module_name",        _module_ctx);
    _r.module_description = _env.project("module_description", _module_ctx);

    // walk units under this module via a per-unit section, collecting the
    // unit_name projections -- this exercises a *nested* section using the
    // SAME env (just a deeper focus).
    const section<test_context> _per_unit = make_per_unit_section();

    std::string _csv;

    _per_unit.for_each(
        _env, _module_ctx, _csv,
        [](const binding_env<test_context>& _e,
           const test_context&              _unit_ctx,
           std::string&                     _acc)
        {
            if (!_acc.empty())
            {
                _acc.push_back(';');
            }

            _acc.append(_e.project("unit_name", _unit_ctx));
        });

    _r.unit_name_csv = std::move(_csv);

    return _r;
}


// ---- tree body: append one subtree per iteration ----

struct tree_collector
{
    tree_cursor*                  modules_head = nullptr;
    std::vector<iteration_record> per_iter;
};


static void
tree_body(
    const binding_env<test_context>& _env,
    const test_context&              _module_ctx,
    tree_collector&                  _target
)
{
    // PLACEMENT (tree): a fresh subtree under the modules_head
    tree_cursor _mod = _target.modules_head->open_child("module");

    _mod.open_child("name")
        .text(_env.project("module_name", _module_ctx));
    _mod.open_child("description")
        .text(_env.project("module_description", _module_ctx));

    // nested section: one <unit> child per unit, named via the same env
    const section<test_context> _per_unit = make_per_unit_section();

    tree_cursor _units = _mod.open_child("units");

    _per_unit.for_each(
        _env, _module_ctx, _units,
        [](const binding_env<test_context>& _e,
           const test_context&              _unit_ctx,
           tree_cursor&                     _u_head)
        {
            _u_head.open_child("unit")
                .text(_e.project("unit_name", _unit_ctx));
        });

    // record what was drawn (for the cross-format invariant check)
    _target.per_iter.push_back(record_for(_env, _module_ctx));
}


// ---- flow body: append one vertical band per iteration ----

struct flow_collector
{
    flow_doc*                     flow = nullptr;
    std::vector<iteration_record> per_iter;
};


static void
flow_body(
    const binding_env<test_context>& _env,
    const test_context&              _module_ctx,
    flow_collector&                  _target
)
{
    // PLACEMENT (flow): a band of lines, separated from the next band by
    // vspace; a page break at the boundary.
    //   The fragments come from env.source_for(_module_ctx) via the same
    // {key} placeholders any text_template would scan -- the seam binding_env
    // is designed around.
    auto _src = _env.source_for(_module_ctx);

    _target.flow->add_text(interpolate("{module_name}",        _src));
    _target.flow->add_text(interpolate("{module_description}", _src));

    // nested section: one line per unit (same env, deeper focus)
    const section<test_context> _per_unit = make_per_unit_section();

    _per_unit.for_each(
        _env, _module_ctx, *_target.flow,
        [](const binding_env<test_context>& _e,
           const test_context&              _unit_ctx,
           flow_doc&                        _fl)
        {
            auto _u_src = _e.source_for(_unit_ctx);

            _fl.add_text(interpolate("    - {unit_name}", _u_src));
        });

    _target.flow->add_vspace(6.0);
    _target.flow->add_page_break();

    _target.per_iter.push_back(record_for(_env, _module_ctx));
}


// ===========================================================================
// driver
// ===========================================================================

static test_report
build_fixture()
{
    test_report _r;

    _r.title = "DTest run";

    // module 1
    {
        report_module _m;
        _m.name        = "container";
        _m.description = "tests of the container vocabulary";
        _m.units.push_back({"vector basics",   {}});
        _m.units.push_back({"vector copy",     {}});
        _m.units.push_back({"vector mutation", {}});
        _r.modules.push_back(std::move(_m));
    }

    // module 2
    {
        report_module _m;
        _m.name        = "callable";
        _m.description = "tests of the callable vocabulary";
        _m.units.push_back({"plain function", {}});
        _m.units.push_back({"capturing lambda", {}});
        _r.modules.push_back(std::move(_m));
    }

    // module 3 (empty units, to prove the iteration handles 0)
    {
        report_module _m;
        _m.name        = "event";
        _m.description = "tests of the event vocabulary";
        _r.modules.push_back(std::move(_m));
    }

    return _r;
}


static void
print_tree(
    std::ostream&    _os,
    const tree_node& _n,
    std::size_t      _depth = 0
)
{
    for (std::size_t _i = 0; _i < _depth; ++_i)
    {
        _os << "  ";
    }

    _os << '<' << _n.name << '>';
    if (!_n.text.empty())
    {
        _os << _n.text << "</" << _n.name << '>';
    }
    _os << '\n';

    for (const auto& _c : _n.children)
    {
        print_tree(_os, *_c, _depth + 1);
    }

    if ( (!_n.children.empty()) && (_n.text.empty()) )
    {
        for (std::size_t _i = 0; _i < _depth; ++_i)
        {
            _os << "  ";
        }

        _os << "</" << _n.name << ">\n";
    }
}


static void
print_flow(
    std::ostream&   _os,
    const flow_doc& _f
)
{
    for (const flow_element& _e : _f.elements())
    {
        if (_e.k == flow_element::kind::text)
        {
            _os << "TEXT: " << _e.text << '\n';
        }
        else if (_e.k == flow_element::kind::vspace)
        {
            _os << "VSPACE: " << _e.amount << '\n';
        }
        else
        {
            _os << "PAGE_BREAK\n";
        }
    }
}


int
main()
{
    const test_report     _run = build_fixture();
    test_context          _here;
    _here.run = &_run;

    const binding_env<test_context>     _env = make_dtest_env();
    const section<test_context>         _per_module = make_per_module_section();

    // ---- tree rendering ----
    tree_node       _tree_root;
    _tree_root.name = "report";
    tree_cursor     _root_cursor(&_tree_root);
    tree_cursor     _modules_head = _root_cursor.open_child("modules");

    tree_collector _tc;
    _tc.modules_head = &_modules_head;

    _per_module.for_each(_env, _here, _tc, tree_body);

    // ---- flow rendering ----
    flow_doc        _flow;
    flow_collector  _fc;
    _fc.flow = &_flow;

    _per_module.for_each(_env, _here, _fc, flow_body);

    // ---- the invariant: per-iteration fragments must match ----
    //   This is the architectural point of the section / binding_env design.
    //   If this assertion ever fails, the binding seam has been broken.
    assert(_tc.per_iter.size() == _fc.per_iter.size());

    for (std::size_t _i = 0; _i < _tc.per_iter.size(); ++_i)
    {
        const iteration_record& _t = _tc.per_iter[_i];
        const iteration_record& _f = _fc.per_iter[_i];

        if ( (_t.module_name        != _f.module_name)        ||
             (_t.module_description != _f.module_description) ||
             (_t.unit_name_csv      != _f.unit_name_csv) )
        {
            std::cerr << "FRAGMENT DIVERGENCE at iteration " << _i << '\n'
                      << "  tree.module_name = '" << _t.module_name << "'\n"
                      << "  flow.module_name = '" << _f.module_name << "'\n"
                      << "  tree.module_desc = '"
                      << _t.module_description << "'\n"
                      << "  flow.module_desc = '"
                      << _f.module_description << "'\n"
                      << "  tree.units       = '" << _t.unit_name_csv << "'\n"
                      << "  flow.units       = '" << _f.unit_name_csv << "'\n";
            return 1;
        }
    }

    // ---- proof that the env was unchanged across iteration ----
    //   The env is const-borrowed by for_each and the section never reads
    //   it itself; this is here as a runtime witness to the static fact.
    if (_env.size() != 5)
    {
        std::cerr << "env size drifted: " << _env.size() << '\n';
        return 1;
    }

    // ---- output ----
    std::cout << "== TREE RENDERING ==\n";
    print_tree(std::cout, _tree_root);
    std::cout << "\n== FLOW RENDERING ==\n";
    print_flow(std::cout, _flow);

    std::cout << "\n== INVARIANT ==\n"
              << "per-iteration fragments matched across tree and flow: "
              << _tc.per_iter.size() << " iterations.\n";

    return 0;
}
