$(this).requires.lib := \
	ast/libast.a \
	symtab/libsymtab.a \
	expr/libexpr.a \
	lexer/liblexer.a \
	gen/libgen.a \
	type/libtype.a \
	util/libutil.a

$(this).CPPFLAGS += -Wno-unused-parameter -I `llvm-config --includedir`
$(this).extra_libs += `llvm-config --ldflags --system-libs --libs all`

# All files in source directory beginning with 'xtest' are optional targets
# (built with 'make opt'). Here we here default targets (built with 'make').

$(this).install := xtest_parser
