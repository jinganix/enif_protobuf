REBAR=$(shell which rebar || echo ./rebar)

all: get-deps compile

get-deps:
	@$(REBAR) get-deps

clean:
	$(REBAR) clean
	rm -rf logs
	rm -rf .eunit
	rm -f test/*.beam
	rm -f test/Emakefile
	rm -f c_src/*.o

compile:
	@$(REBAR) compile

distclean: clean
	git clean -fxd

build:
	$(REBAR) compile

ct:
	./scripts/generate_emakefile.escript
	@$(REBAR) skip_deps=true ct

eunit:
	@$(REBAR) skip_deps=true eunit

test: eunit

check: build eunit

%.beam: %.erl
	erlc -o test/ $<

.PHONY: all clean distclean depends build etap eunit check
