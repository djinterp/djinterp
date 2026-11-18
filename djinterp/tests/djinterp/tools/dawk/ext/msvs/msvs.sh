#!/bin/sh
# msvs.sh -- the -x msvs extension and its parts, -x sln and -x vcxproj,
# msvs_generate.awk and msvs_survey.awk, against outputs fixed in advance;
# then the reader and the document layer over real files, and the files
# generated against an XML parser of Python's own.
#
# Each case runs dawk over files written here and compares what it prints,
# its status, and, when it writes, the bytes it wrote. The generator is run
# over a small tree made here: tests globbed, kept by if(EXISTS) in a
# foreach, made by a function that sets PARENT_SCOPE, given MSVC's options
# under if(MSVC), and one no aggregate reaches. After the cases: every
# Visual Studio file under build/cmake read whole, when the tree has them;
# random changes made to some of them, each proved to touch only what it
# said; and, when python3 is installed, the generated tree proved sound by
# msvs_tree_check.py.
#
#   sh tests/djinterp/tools/dawk/ext/msvs/msvs.sh [build/dawk] [build/test_dmsvs]
#                                        [build/test_dmsvs_doc]
#
# The document layer's test is looked for beside the reader's when it is
# not named.
DAWK=${1:-build/dawk}
READER=${2:-build/test_dmsvs}
DOC=${3:-$(dirname "$READER")/test_dmsvs_doc}
HERE=tests/djinterp/tools/dawk/ext/msvs
RES=res/djinterp/tools/dawk/ext/msvs
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
pass=0
fail=0

# same NAME WANT ARGS...: dawk's output and status must be WANT exactly,
# its last line the status
same() {
    name=$1
    want=$2
    shift 2
    got=$("$DAWK" "$@" 2>&1; echo "status $?")
    if [ "$got" = "$want" ]; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $name"
        printf '%s\n' "$want" | sed 's/^/    want: /'
        printf '%s\n' "$got"  | sed 's/^/    got:  /'
    fi
}

# holds NAME FILE WANT-FILE: the file must hold the bytes WANT-FILE does
holds() {
    if cmp -s "$2" "$3"; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $1"
        diff "$3" "$2" | sed 's/^/    /'
    fi
}

# crlf FILE: the file's line endings made CR LF
crlf() {
    sed 's/$/\r/' "$1" > "$1.crlf" && mv "$1.crlf" "$1"
}

# check NAME COMMAND...: the command must succeed
check() {
    name=$1
    shift
    if out=$("$@" 2>&1); then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
        echo "FAIL $name"
        printf '%s\n' "$out" | tail -20 | sed 's/^/    /'
    fi
}

# -- vcxproj: reading ---------------------------------------------------------
cat > "$work/p.vcxproj" <<'EOF'
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Label="Globals">
    <ProjectGuid>{11111111-2222-3333-4444-555555555555}</ProjectGuid>
    <ProjectName>p</ProjectName>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <PlatformToolset>ClangCL</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="exists('x')">
    <OutDir>a &amp; b</OutDir>
  </PropertyGroup>
  <ItemGroup>
    <ClCompile Include="src\b.c" />
    <ClCompile Include="src\d.c" />
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
</Project>
EOF
cat > "$work/read.awk" <<'EOF'
BEGIN {
    p = vcxproj_open(file)
    print "root", p, vcxproj_name(p), vcxproj_kind(p), vcxproj_children(p),
          vcxproj_line(p)
    for (e = vcxproj_find(p, "PlatformToolset"); e;
         e = vcxproj_find(p, "PlatformToolset", e))
        print "toolset", vcxproj_text(e), "line", vcxproj_line(e), "depth",
              vcxproj_depth(e), "held to",
              "[" vcxproj_condition(vcxproj_parent(e)) "]",
              vcxproj_applies(e, "Debug|x64"), vcxproj_applies(e, "Release|x64")
    g = vcxproj_find(p, "PropertyGroup",
                     vcxproj_find(p, "PropertyGroup", vcxproj_first(p)))
    print "unknown", "[" vcxproj_condition(g) "]", vcxproj_applies(g, "Debug|x64"),
          vcxproj_text(vcxproj_first(g))
    print "property", vcxproj_property(p, "PlatformToolset"),
          vcxproj_property(p, "PlatformToolset", "Debug|x64"),
          vcxproj_property(p, "PlatformToolset", "debug|X64"),
          vcxproj_property(p, "PlatformToolset", "Release|Win32"),
          "[" vcxproj_property(p, "Nothing") "]"
    print "item", vcxproj_attr(vcxproj_item(p, "ClCompile", "SRC/B.C"), "Include"),
          vcxproj_item(p, "ClCompile", "src/c.c"),
          vcxproj_item(p, "ClInclude", "src\\b.c")
    print "attr", "[" vcxproj_attr(p, "DefaultTargets") "]", vcxproj_has(p, "xmlns"),
          vcxproj_has(p, "Nope"), "[" vcxproj_attr(p, "Nope") "]"
    print "text of a parent", "[" vcxproj_text(p) "]"
    print "relations", vcxproj_parent(p), vcxproj_name(vcxproj_next(vcxproj_first(p))),
          (vcxproj_root(vcxproj_first(vcxproj_first(p))) == p)
    print "nothing", "[" vcxproj_name(999) "]", vcxproj_parent(0),
          vcxproj_find(77, "x"), "[" vcxproj_path(55) "]"
    print "missing", vcxproj_open("nope.vcxproj"), vcxproj_error()
}
EOF
same 'a project read: elements, conditions, properties, items' "root 1 Project project 5 2
toolset v143 line 6 depth 2 held to [] 1 1
toolset ClangCL line 9 depth 2 held to [Debug|x64] 1 0
unknown [?] 2 a & b
property v143 ClangCL ClangCL v143 []
item src\\b.c 0 0
attr [Build] 1 0 []
text of a parent []
relations 0 PropertyGroup 1
nothing [] 0 0 []
missing -1 cannot read: nope.vcxproj
status 0" \
    -x vcxproj -v file="$work/p.vcxproj" -f "$work/read.awk"

# -- vcxproj: changes ---------------------------------------------------------
cp "$work/p.vcxproj" "$work/q.vcxproj"
cat > "$work/change.awk" <<'EOF'
BEGIN {
    p = vcxproj_open(file)
    g = vcxproj_first(p)
    print "set", vcxproj_set_text(vcxproj_find(g, "ProjectName"), "q & r")
    print "attr", vcxproj_set_attr(g, "Label", "Globals \"main\""),
          vcxproj_set_attr(g, "Id", "1"), vcxproj_remove_attr(g, "Id"),
          vcxproj_remove_attr(g, "Id")
    print "add", vcxproj_name(vcxproj_add(g, "RootNamespace", "q")),
          vcxproj_name(vcxproj_insert(vcxproj_first(g), "Keyword", "Win32Proj"))
    print "items", (vcxproj_add_item(p, "ClCompile", "src\\c.c") > 0),
          (vcxproj_add_item(p, "ClCompile", "src\\a.c") > 0),
          (vcxproj_add_item(p, "ClCompile", "src\\e.c") > 0),
          (vcxproj_add_item(p, "ClInclude", "inc\\a.h") > 0)
    a = vcxproj_add_item(p, "ClCompile", "SRC/A.C")
    print "again", (a == vcxproj_item(p, "ClCompile", "src\\a.c"))
    print "remove", vcxproj_remove_item(p, "ClCompile", "src\\d.c"),
          vcxproj_remove_item(p, "ClCompile", "src\\d.c")
    u = vcxproj_find(p, "PropertyGroup", g)
    print "remove element", vcxproj_remove(u), "[" vcxproj_name(u) "]",
          vcxproj_remove(u), vcxproj_error()
    print "changes", vcxproj_changes(p)
    print "refused", vcxproj_add(p, "1x"), vcxproj_set_attr(p, "a b", "v"),
          vcxproj_add(p, "a:b"), vcxproj_set_text(p, "x"), vcxproj_remove(p),
          vcxproj_insert(p, "X")
    print "why", vcxproj_error()
    print "save", vcxproj_save(p), vcxproj_save(p), vcxproj_save(p, "q.sln"),
          vcxproj_error()
}
EOF
same 'changes made at once, each read again, refusals said' "set 1
attr 1 1 1 0
add RootNamespace Keyword
items 1 1 1 1
again 1
remove 1 0
remove element 1 [] -1 no such element
changes 13
refused -1 -1 -1 -1 -1 -1
why the root has no siblings
save 1 2 -1 not a file of the kind q.sln names: line 2, column 1: a solution's root must be <Solution>
status 0" \
    -x vcxproj -v file="$work/q.vcxproj" -f "$work/change.awk"
cat > "$work/q.want" <<'EOF'
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Label="Globals &quot;main&quot;">
    <Keyword>Win32Proj</Keyword>
    <ProjectGuid>{11111111-2222-3333-4444-555555555555}</ProjectGuid>
    <ProjectName>q &amp; r</ProjectName>
    <PlatformToolset>v143</PlatformToolset>
    <RootNamespace>q</RootNamespace>
  </PropertyGroup>
  <PropertyGroup Condition="exists('x')">
    <OutDir>a &amp; b</OutDir>
  </PropertyGroup>
  <ItemGroup>
    <ClCompile Include="src\a.c" />
    <ClCompile Include="src\b.c" />
    <ClCompile Include="src\c.c" />
    <ClCompile Include="src\e.c" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="inc\a.h" />
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
</Project>
EOF
holds '...the file as changed, every other byte its own' "$work/q.vcxproj" "$work/q.want"

# -- vcxproj: new files, from nothing and from a template --------------------
cat > "$work/new.awk" <<'EOF'
BEGIN {
    f = vcxproj_new(dir "/made/deep/x.vcxproj.filters")
    print "filters", f, vcxproj_kind(f)
    print "item", (vcxproj_add_item(f, "ClCompile", "..\\a.c", "Source Files\\src") > 0),
          (vcxproj_add_item(f, "ClInclude", "..\\a.h", "Header Files") > 0)
    print "moved", (vcxproj_add_item(f, "ClCompile", "..\\a.c", "Source Files") > 0)
    print "save", vcxproj_save(f)
    u = vcxproj_new(dir "/made/x.vcxproj.user")
    vcxproj_add(u, "PropertyGroup")
    print "user", vcxproj_save(u)
    t = vcxproj_new(dir "/made/t.vcxproj", dir "/p.vcxproj")
    print "template", vcxproj_property(t, "ProjectName"), vcxproj_path(t)
    print "refused", vcxproj_new(dir "/x.sln"), vcxproj_error()
    print "no template", vcxproj_new(dir "/y.vcxproj", dir "/none")
}
EOF
same 'new files: filters made, filters moved, a user file, a template' "filters 1 filters
item 1 1
moved 1
save 1
user 1
template p $work/made/t.vcxproj
refused -1 a solution is made with sln_new
no template -1
status 0" \
    -x vcxproj -v dir="$work" -f "$work/new.awk"
{
    printf '\357\273\277'
    cat <<'EOF'
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup>
    <Filter Include="Header Files">
      <UniqueIdentifier>{2e82be22-b843-50a1-a00c-2d7531a59b7f}</UniqueIdentifier>
    </Filter>
    <Filter Include="Source Files">
      <UniqueIdentifier>{7b79b57c-8c38-5da4-bbf0-da835ba16e22}</UniqueIdentifier>
    </Filter>
    <Filter Include="Source Files\src">
      <UniqueIdentifier>{49c0c857-8e7f-5a80-ad10-684da0f2ecc2}</UniqueIdentifier>
    </Filter>
  </ItemGroup>
  <ItemGroup>
    <ClCompile Include="..\a.c">
      <Filter>Source Files</Filter>
    </ClCompile>
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="..\a.h">
      <Filter>Header Files</Filter>
    </ClInclude>
  </ItemGroup>
</Project>
EOF
} | awk 'BEGIN { ORS = "" } NR > 1 { print "\r\n" } { print }' > "$work/filters.want2"
holds '...the filters as Visual Studio writes them: a mark, CR LF, no last line ending' \
    "$work/made/deep/x.vcxproj.filters" "$work/filters.want2"
printf '\357\273\277<?xml version="1.0" encoding="utf-8"?>\r\n<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">\r\n  <PropertyGroup />\r\n</Project>' \
    > "$work/user.want"
holds '...the user file as Visual Studio writes one' "$work/made/x.vcxproj.user" "$work/user.want"

# -- sln: both forms ----------------------------------------------------------
mkdir -p "$work/a"
cp "$work/p.vcxproj" "$work/a/a.vcxproj"
printf '\357\273\277\r\nMicrosoft Visual Studio Solution File, Format Version 12.00\r\n# Visual Studio Version 17\r\nVisualStudioVersion = 17.14.36327.8\r\nMinimumVisualStudioVersion = 10.0.40219.1\r\nProject("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "p", "p.vcxproj", "{AAAAAAAA-0000-0000-0000-000000000001}"\r\nEndProject\r\nGlobal\r\n\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\r\n\t\tDebug|x64 = Debug|x64\r\n\t\tDebug|x86 = Debug|x86\r\n\tEndGlobalSection\r\n\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n\t\t{AAAAAAAA-0000-0000-0000-000000000001}.Debug|x64.ActiveCfg = Debug|x64\r\n\t\t{AAAAAAAA-0000-0000-0000-000000000001}.Debug|x64.Build.0 = Debug|x64\r\n\t\t{AAAAAAAA-0000-0000-0000-000000000001}.Debug|x86.ActiveCfg = Debug|Win32\r\n\t\t{AAAAAAAA-0000-0000-0000-000000000001}.Debug|x86.Build.0 = Debug|Win32\r\n\tEndGlobalSection\r\n\tGlobalSection(SolutionProperties) = preSolution\r\n\t\tHideSolutionNode = FALSE\r\n\tEndGlobalSection\r\nEndGlobal\r\n' \
    > "$work/s.sln"
printf '<Solution>\r\n  <Configurations>\r\n    <Platform Name="x64" />\r\n    <Platform Name="x86" />\r\n  </Configurations>\r\n  <Project Path="p.vcxproj" Id="aaaaaaaa-0000-0000-0000-000000000001" />\r\n</Solution>\r\n' \
    > "$work/s.slnx"
cat > "$work/sln.awk" <<'EOF'
function show(s,    i, k, line) {
    printf "%s: %s, %d projects, %d folders, %d configs\n", sln_path(s),
           sln_form(s), sln_projects(s), sln_folders(s), sln_configs(s)
    for (i = 1; i <= sln_projects(s); i++) {
        line = sprintf("  %d %s %s [%s] {%s} line %d", i,
                       sln_project(s, i, "name"), sln_project(s, i, "path"),
                       sln_project(s, i, "folder"), sln_project(s, i, "id"),
                       sln_project(s, i, "line"))
        for (k = 1; k <= sln_dependencies(s, i); k++)
            line = line " after " sln_dependency(s, i, k)
        print line
    }
    for (i = 1; i <= sln_folders(s); i++)
        print "  folder " sln_folder(s, i)
    for (i = 1; i <= sln_configs(s); i++)
        print "  config " sln_config(s, i)
}
BEGIN {
    s = sln_open(file)
    show(s)
    print "find", sln_find(s, "P.VCXPROJ"), sln_find(s, "x.vcxproj")
    print "add", sln_add_project(s, "a" sep "a.vcxproj", "tools\\dawk"),
          sln_add_folder(s, "/empty/"),
          sln_add_dependency(s, "p.vcxproj", "a" sep "a.vcxproj"),
          sln_add_config(s, "Release|x64")
    print "refused", sln_add_dependency(s, "p.vcxproj", "z.vcxproj"),
          sln_add_config(s, "Release"), sln_add_config(s, "Release|*"),
          sln_error()
    show(s)
    print "save", sln_save(s, out), sln_save(s)
    print "remove", sln_remove_config(s, "*|x86"),
          sln_remove_dependency(s, "p.vcxproj", "a" sep "a.vcxproj"),
          sln_remove_project(s, "a" sep "a.vcxproj"),
          sln_remove_project(s, "a" sep "a.vcxproj")
    print "whole", sln_remove_config(s, "Debug|x64"), sln_error()
    show(s)
    sln_save(s, out2)
    print "nothing", sln_projects(99), "[" sln_project(s, 9, "name") "]",
          "[" sln_config(s, 0) "]", sln_save(42), sln_error()
}
EOF
same 'a .sln read and changed: projects, folders, dependencies, configurations' "$work/s.sln: sln, 1 projects, 0 folders, 2 configs
  1 p p.vcxproj [] {aaaaaaaa-0000-0000-0000-000000000001} line 6
  config Debug|x64
  config Debug|x86
find 1 0
add 2 1 1 1
refused -1 -1 -1 a configuration is \"Debug|x64\"
$work/s.sln: sln, 2 projects, 3 folders, 3 configs
  1 p p.vcxproj [] {aaaaaaaa-0000-0000-0000-000000000001} line 6 after a\\a.vcxproj
  2 a a\\a.vcxproj [/tools/dawk/] {11111111-2222-3333-4444-555555555555} line 15
  folder /tools/
  folder /tools/dawk/
  folder /empty/
  config Debug|x64
  config Debug|x86
  config Release|x64
save 1 2
remove 1 1 1 0
whole 1 a configuration is \"Debug|x64\"
$work/t.sln: sln, 1 projects, 3 folders, 1 configs
  1 p p.vcxproj [] {aaaaaaaa-0000-0000-0000-000000000001} line 6
  folder /tools/
  folder /tools/dawk/
  folder /empty/
  config Release|x64
nothing 0 [] [] -1 no such solution
status 0" \
    -x sln -v file="$work/s.sln" -v 'sep=\\' -v out="$work/t.sln" \
    -v out2="$work/u.sln" -f "$work/sln.awk"
{
    printf '\357\273\277\n'
    cat <<'EOF'
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
VisualStudioVersion = 17.14.36327.8
MinimumVisualStudioVersion = 10.0.40219.1
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "p", "p.vcxproj", "{AAAAAAAA-0000-0000-0000-000000000001}"
EndProject
Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "tools", "tools", "{61FAAA2B-7518-5E46-9829-4D169DE0802A}"
EndProject
Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "dawk", "dawk", "{2D83B311-73AE-5203-9FBA-8B236790B126}"
EndProject
Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "empty", "empty", "{4B27411E-0FE8-5AA8-9F60-9C7DE0065B85}"
EndProject
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Release|x64 = Release|x64
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
		{AAAAAAAA-0000-0000-0000-000000000001}.Release|x64.ActiveCfg = Release|x64
		{AAAAAAAA-0000-0000-0000-000000000001}.Release|x64.Build.0 = Release|x64
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(NestedProjects) = preSolution
		{2D83B311-73AE-5203-9FBA-8B236790B126} = {61FAAA2B-7518-5E46-9829-4D169DE0802A}
	EndGlobalSection
EndGlobal
EOF
} > "$work/u.want"
crlf "$work/u.want"
holds '...its lines spliced, CR LF and mark kept, empty sections gone' "$work/u.sln" "$work/u.want"
same 'a .slnx read and changed the same way' "$work/s.slnx: slnx, 1 projects, 0 folders, 4 configs
  1 p p.vcxproj [] {aaaaaaaa-0000-0000-0000-000000000001} line 6
  config Debug|x64
  config Debug|x86
  config Release|x64
  config Release|x86
find 1 0
add 1 1 1 1
refused -1 -1 -1 a configuration is \"Debug|x64\"
$work/s.slnx: slnx, 2 projects, 3 folders, 4 configs
  1 a a/a.vcxproj [/tools/dawk/] {} line 9
  2 p p.vcxproj [] {aaaaaaaa-0000-0000-0000-000000000001} line 11 after a/a.vcxproj
  folder /empty/
  folder /tools/
  folder /tools/dawk/
  config Debug|x64
  config Debug|x86
  config Release|x64
  config Release|x86
save 1 2
remove 1 1 1 0
whole -1 a .slnx builds every build type on every platform: remove \"Debug|*\" or \"*|x64\"
$work/t.slnx: slnx, 1 projects, 3 folders, 2 configs
  1 p p.vcxproj [] {aaaaaaaa-0000-0000-0000-000000000001} line 10
  folder /empty/
  folder /tools/
  folder /tools/dawk/
  config Debug|x64
  config Release|x64
nothing 0 [] [] -1 no such solution
status 0" \
    -x sln -v file="$work/s.slnx" -v 'sep=/' -v out="$work/t.slnx" \
    -v out2="$work/u.slnx" -f "$work/sln.awk"
cat > "$work/u.slnx.want" <<'EOF'
<Solution>
  <Configurations>
    <BuildType Name="Debug" />
    <BuildType Name="Release" />
    <Platform Name="x64" />
  </Configurations>
  <Folder Name="/empty/" />
  <Folder Name="/tools/" />
  <Folder Name="/tools/dawk/" />
  <Project Path="p.vcxproj" Id="aaaaaaaa-0000-0000-0000-000000000001" />
</Solution>
EOF
crlf "$work/u.slnx.want"
holds '...its elements, the build types it built written out before one went' \
    "$work/u.slnx" "$work/u.slnx.want"
same 'a solution made new, in each form' "1 1 1 3
status 0" \
    -x sln -v dir="$work/new" 'BEGIN {
        a = sln_new(dir "/n.sln"); b = sln_new(dir "/n.slnx")
        sln_add_project(a, "x\\x.vcxproj"); sln_add_project(b, "x/x.vcxproj")
        print (sln_save(a) == 1), (sln_save(b) == 1), sln_projects(a), sln_open(dir "/n.slnx") }'
check '...each reads back whole' "$READER" --corpus "$work/new"

# -- msvs: the umbrella's own -------------------------------------------------
mkdir -p "$work/g/sub" "$work/g/.hidden"
touch "$work/g/a.cpp" "$work/g/b.cpp" "$work/g/b.hpp" "$work/g/sub/c.cpp" "$work/g/.h.cpp"
same 'guids, relative paths, globs, kinds' "c8a9f80b-5306-5530-997b-7ae52b9a8be3
..\\..\\..\\djinterp.props c/d.vcxproj . []
2 1 0
solution filters user props project other
[$work/g/a.cpp
$work/g/b.cpp]
[$work/g/sub]
[$work/g/sub/c.cpp]
[$work/g/b.cpp
$work/g/b.hpp]
[$work/g/.h.cpp]
[]
status 0" \
    -x msvs -v dir="$work" 'BEGIN {
        print msvs_guid("abc")
        print msvs_relative(".config/.msvs/testing/a/b", ".config/.msvs/djinterp.props"),
              msvs_relative("a/b", "a/b/c/d.vcxproj", "/"), msvs_relative("a", "a"),
              "[" msvs_relative("../x", "y") "]"
        print msvs_exists(dir "/g"), msvs_exists(dir "/g/a.cpp"), msvs_exists(dir "/none")
        print msvs_kind("x.SLNX"), msvs_kind("x.vcxproj.filters"), msvs_kind("x.vcxproj.user"),
              msvs_kind("d.props"), msvs_kind("x.vcxproj"), msvs_kind("x.txt")
        print "[" msvs_glob(dir "/g/*.cpp") "]"
        print "[" msvs_glob(dir "/g/*", "dirs") "]"
        print "[" msvs_glob(dir "/g/*/*.cpp") "]"
        print "[" msvs_glob(dir "/g/[!a]?[ch]pp", "all") "]"
        print "[" msvs_glob(dir "/g/.*.cpp") "]"
        print "[" msvs_glob(dir "/none/*") "]" }'
same 'the parts alone, and beside the umbrella' "1 1 1
status 0" \
    -x sln -x vcxproj -x msvs -x sln 'BEGIN { print (sln_new("a.slnx") > 0), (vcxproj_new("a.props") > 0), (msvs_guid("") != "") }'

# -- msvs_generate.awk --------------------------------------------------------
T="$work/tree"
C="$T/build/cmake/config"
mkdir -p "$C/testing/djinterp/thing" "$C/testing/djinterp/suites" \
         "$C/testing/hidden/deep" "$T/inc/djinterp/thing" \
         "$T/tests/djinterp/thing" "$T/src/djinterp/c" "$T/.config/.msvs" \
         "$T/res/djinterp/tools/dawk/ext/msvs"
cp "$RES/msvs_test.vcxproj" "$T/res/djinterp/tools/dawk/ext/msvs/"
printf '<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003" />\n' \
    > "$T/.config/.msvs/djinterp.props"
cat > "$C/CMakeLists.txt" <<'EOF'
get_filename_component(DJINTERP_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(DJINTERP_SRC_ROOT "${DJINTERP_PROJECT_ROOT}/src" CACHE PATH "" FORCE)
add_library(djinterp_c_runtime STATIC "${DJINTERP_SRC_ROOT}/djinterp/c/djinterp.c")
set_target_properties(djinterp_c_runtime PROPERTIES FOLDER "djinterp/runtime")
add_subdirectory(testing)
EOF
echo 'djinterp_add_test_subtree(all_tests)' > "$C/testing/CMakeLists.txt"
echo 'djinterp_add_test_subtree(all_djinterp)' > "$C/testing/djinterp/CMakeLists.txt"
cat > "$C/testing/djinterp/thing/CMakeLists.txt" <<'EOF'
set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/thing")
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/thing_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")
foreach(_s "${_tests_dir}/extra.cpp" "${_tests_dir}/absent.cpp")
    if(EXISTS "${_s}")
        list(APPEND _sources "${_s}")
    endif()
endforeach()
djinterp_add_test_executable(thing_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/thing_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/thing"
        "${_tests_dir}")
if(MSVC)
    target_compile_options(thing_tests PRIVATE /utf-8 -Wall)
    target_compile_definitions(thing_tests PRIVATE THING=1 D_TESTING=1)
else()
    target_compile_definitions(thing_tests PRIVATE NOT_MSVC=1)
endif()
EOF
cat > "$C/testing/djinterp/suites/CMakeLists.txt" <<'EOF'
function(add_suite _name)
    djinterp_add_test_executable(${_name}
        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${_name}_runner.cpp")
    set(_made "${_made};${_name}" PARENT_SCOPE)
endfunction()
add_suite(one_tests)
add_suite(two_tests)
if(NOT _made STREQUAL ";one_tests;two_tests")
    message(FATAL_ERROR "scope")
endif()
djinterp_add_test_executable(three_tests SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/one_tests_runner.cpp")
while(FALSE)
endwhile()
EOF
echo 'djinterp_add_test_executable(hidden_tests SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/r.cpp")' \
    > "$C/testing/hidden/deep/CMakeLists.txt"
for f in "$C/testing/hidden/deep/r.cpp" \
         "$C/testing/djinterp/thing/thing_tests_runner.cpp" \
         "$C/testing/djinterp/suites/one_tests_runner.cpp" \
         "$C/testing/djinterp/suites/two_tests_runner.cpp" \
         "$T/inc/djinterp/thing/thing.h" "$T/inc/djinterp/thing/thing.hpp" \
         "$T/tests/djinterp/thing/thing_tests.hpp" \
         "$T/tests/djinterp/thing/thing_tests_a.cpp" \
         "$T/tests/djinterp/thing/thing_tests_b.cpp" \
         "$T/tests/djinterp/thing/thing_tests_main.cpp" \
         "$T/tests/djinterp/thing/extra.cpp" \
         "$T/tests/djinterp/thing/other.hpp" "$T/src/djinterp/c/djinterp.c"; do
    : > "$f"
done
GEN="-x cmake -x msvs -f $RES/msvs_generate.awk -v root=$T"
same 'the tree generated: the tests CMake reaches, its aggregates, the runtime' \
"msvs_generate: build/cmake/config/testing/djinterp/suites/CMakeLists.txt: not followed: while@12
4 tests, 2 aggregates, 1 libraries: 20 written, 0 unchanged
status 0" $GEN
same '...and generated again, unchanged' \
"msvs_generate: build/cmake/config/testing/djinterp/suites/CMakeLists.txt: not followed: while@12
4 tests, 2 aggregates, 1 libraries: 0 written, 16 unchanged
status 0" $GEN
M="$T/.config/.msvs"
grep -o 'Include="[^"]*"' "$M/testing/djinterp/thing/thing_tests.vcxproj" > "$work/items"
cat > "$work/items.want" <<'EOF'
Include="Debug|Win32"
Include="Release|Win32"
Include="Debug|x64"
Include="Release|x64"
Include="..\..\..\..\..\build\cmake\config\testing\djinterp\thing\thing_tests_runner.cpp"
Include="..\..\..\..\..\tests\djinterp\thing\extra.cpp"
Include="..\..\..\..\..\tests\djinterp\thing\thing_tests_a.cpp"
Include="..\..\..\..\..\tests\djinterp\thing\thing_tests_b.cpp"
Include="..\..\..\..\..\inc\djinterp\thing\thing.h"
Include="..\..\..\..\..\inc\djinterp\thing\thing.hpp"
Include="..\..\..\..\..\tests\djinterp\thing\thing_tests.hpp"
Include="..\..\..\modules\djinterp\runtime\djinterp_c_runtime.vcxproj"
EOF
holds '...a test: its globbed sources, those if(EXISTS) kept, its headers, the runtime' \
    "$work/items" "$work/items.want"
tr -d '\r' < "$M/testing/djinterp/thing/thing_tests.vcxproj" \
    | grep -E 'djinterp\.props|<ProjectName|<DTestPath|AdditionalOptions|THING' \
    | sed 's/D_TEST_OUTPUT_DIR.*//' | sort -u > "$work/settings"
cat > "$work/settings.want" <<'EOF'
      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
      <PreprocessorDefinitions>THING=1;NDEBUG;_CONSOLE;_UNICODE;UNICODE;D_TESTING=1;
      <PreprocessorDefinitions>THING=1;_DEBUG;_CONSOLE;_UNICODE;UNICODE;D_TESTING=1;
    <DTestPath>testing\djinterp\thing</DTestPath>
    <Import Project="..\..\..\djinterp.props" />
    <ProjectName>thing_tests</ProjectName>
EOF
holds "...its settings: the props from where it is, MSVC's options alone, no definition twice" \
    "$work/settings" "$work/settings.want"
tr -d '\r' < "$M/testing/all_tests.slnx" > "$work/all"
cat > "$work/all.want" <<'EOF'
<Solution>
  <Configurations>
    <BuildType Name="Debug" />
    <BuildType Name="Release" />
    <Platform Name="x64" />
    <Platform Name="x86" />
  </Configurations>
  <Folder Name="/djinterp/">
    <Project Path="djinterp/suites/one_tests.vcxproj" Id="8f14257d-6d52-5271-b930-a51ac4fc5aad" />
    <Project Path="djinterp/suites/three_tests.vcxproj" Id="0e3f1fc6-6fcd-5760-9f9d-0f5c8f57c28e" />
    <Project Path="djinterp/suites/two_tests.vcxproj" Id="9d0d6f5e-7258-57aa-9ad8-f90c0a934d48" />
    <Project Path="djinterp/thing/thing_tests.vcxproj" Id="65380207-7a31-51bd-98f9-14e9c0b10031" />
  </Folder>
  <Folder Name="/runtime/">
    <Project Path="../modules/djinterp/runtime/djinterp_c_runtime.vcxproj" Id="09f54de4-9ce1-5c56-8205-e4ba9463780c" />
  </Folder>
</Solution>
EOF
holds '...an aggregate: every test beneath it, by its own id, in its folder' \
    "$work/all" "$work/all.want"
tr -d '\r' < "$M/modules/djinterp/runtime/djinterp_c_runtime.vcxproj" \
    | grep -cE 'StaticLibrary|D_TESTING|<Link>|DFrameworkLibRoot' > "$work/runtime"
echo 5 > "$work/runtime.want"
holds '...the runtime: a static library, without the tests'"'"' settings' \
    "$work/runtime" "$work/runtime.want"
same '...every test, with all=1; only a part, with only=; nothing written, with dry=1' \
"msvs_generate: build/cmake/config/testing/djinterp/suites/CMakeLists.txt: not followed: while@12
would write $M/modules/djinterp/runtime/djinterp_c_runtime.vcxproj
would write $M/modules/djinterp/runtime/djinterp_c_runtime.vcxproj.filters
would write $M/testing/hidden/deep/hidden_tests.vcxproj
would write $M/testing/hidden/deep/hidden_tests.vcxproj.filters
would write $M/testing/hidden/deep/hidden_tests.vcxproj.user
would write $M/testing/hidden/deep/hidden_tests.slnx
1 tests, 0 aggregates, 1 libraries: (dry run) 6 written, 0 unchanged
status 0" $GEN -v all=1 -v only=hidden -v dry=1
check '...and nothing was' test ! -e "$M/testing/hidden"
same '...the other form of solution, with form=sln' \
"msvs_generate: build/cmake/config/testing/djinterp/suites/CMakeLists.txt: not followed: while@12
4 tests, 2 aggregates, 1 libraries: 6 written, 10 unchanged
status 0" $GEN -v form=sln
check '...every file it made reads whole' "$READER" --corpus "$M"
same 'the generated tree surveyed: nothing stale' \
"27 files: 12 solutions, 5 projects, 5 filters, 4 user, 1 props; 0 stale
status 0" -x msvs -f "$RES/msvs_survey.awk" -v quiet=1 "$M/testing" "$M/modules" "$M/djinterp.props"

# -- msvs_survey.awk ----------------------------------------------------------
S="$work/stale"
mkdir -p "$S/lib"
cp "$work/p.vcxproj" "$S/lib/lib.vcxproj"
: > "$S/here.c"
cat > "$S/app.vcxproj" <<'EOF'
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|x64" />
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <ProjectName>app</ProjectName>
    <ProjectGuid>{BBBBBBBB-0000-0000-0000-000000000002}</ProjectGuid>
  </PropertyGroup>
  <PropertyGroup Label="Configuration">
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <ImportGroup Label="PropertySheets">
    <Import Project="..\djinterp.props" />
    <Import Project="$(UserRootDir)\x.props" />
  </ImportGroup>
  <ItemGroup>
    <ClCompile Include="here.c" />
    <ClCompile Include="gone.c" />
    <ClCompile Include="$(Dir)\any.c" />
    <ClInclude Include="" />
  </ItemGroup>
  <ItemGroup>
    <ProjectReference Include="lib\lib.vcxproj">
      <Project>{99999999-0000-0000-0000-000000000009}</Project>
    </ProjectReference>
    <ProjectReference Include="nowhere\x.vcxproj" />
  </ItemGroup>
</Project>
EOF
cat > "$S/app.vcxproj.filters" <<'EOF'
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup>
    <Filter Include="Source Files" />
  </ItemGroup>
  <ItemGroup>
    <ClCompile Include="here.c"><Filter>Source Files</Filter></ClCompile>
    <ClCompile Include="other.c"><Filter>Lost Files</Filter></ClCompile>
  </ItemGroup>
</Project>
EOF
printf '<Solution>\n  <Project Path="app.vcxproj" Id="bbbbbbbb-0000-0000-0000-000000000002" />\n  <Project Path="lib/lib.vcxproj" Id="12345678-0000-0000-0000-000000000000" />\n  <Project Path="away/away.vcxproj" />\n</Solution>\n' \
    > "$S/all.slnx"
printf 'not xml' > "$S/broken.vcxproj"
same 'what is stale, file by file' "$S/all.slnx: slnx, 3 projects, 0 folders; configurations from its projects
    lists lib/lib.vcxproj by another id
    lists away/away.vcxproj, which is not there
$S/app.vcxproj: project app; toolsets v143; Debug|x64; 3 ClCompile, 1 ClInclude, 2 ProjectReference; imports ..\\djinterp.props
    imports ..\\djinterp.props, which is not there
    refers to lib\\lib.vcxproj by another id
    refers to nowhere\\x.vcxproj, which is not there
    2 items name files that are not there
$S/app.vcxproj.filters: filters; 2 ClCompile, 1 Filter
    1 item names a file that is not there
    shows other.c in Lost Files, which it does not define
    1 item its project does not list
    3 items of its project's not shown
$S/broken.vcxproj: project, which does not read
    line 1, column 1: an MSBuild file must be XML
$S/lib/lib.vcxproj: project p; toolsets v143 ClangCL; 2 ClCompile
    2 items name files that are not there
5 files: 1 solutions, 3 projects, 1 filters, 0 user, 0 props; 12 stale
status 2" -x msvs -f "$RES/msvs_survey.awk" "$S"
same '...only what is stale, with quiet=1' "$S/all.slnx
    lists lib/lib.vcxproj by another id
    lists away/away.vcxproj, which is not there
$S/lib/lib.vcxproj
    2 items name files that are not there
2 files: 1 solutions, 1 projects, 0 filters, 0 user, 0 props; 3 stale
status 1" -x msvs -f "$RES/msvs_survey.awk" -v quiet=1 "$S/all.slnx" "$S/lib"

# -- the reader and the documents, over real files ----------------------------
if [ -d build/cmake/msvs ]; then
    check 'the reader over build/cmake' "$READER" --corpus build/cmake
    check 'random changes to its projects, each touching only what it said' \
        "$DOC" --fuzz 1 build/cmake/msvs/Testing/djinterp/test
fi
if command -v python3 > /dev/null 2>&1; then
    check 'the generated tree, read by Python' python3 -I "$HERE/msvs_tree_check.py" "$T"
    for f in "$M/testing/djinterp/thing/thing_tests.vcxproj" \
             "$M/testing/djinterp/thing/thing_tests.vcxproj.filters" \
             "$M/testing/all_tests.slnx" "$work/q.vcxproj" "$work/u.slnx"; do
        if [ "$("$READER" --dump "$f" 2>&1)" = "$(python3 -I "$HERE/xml_tree.py" "$f" 2>&1)" ]; then
            pass=$((pass + 1))
        else
            fail=$((fail + 1))
            echo "FAIL the reader and Python read $f differently"
        fi
    done
else
    echo "note: python3 is not installed; the files made are not read by it"
fi

echo
echo "msvs: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
