REBAR=$(shell which rebar3 || echo ./rebar3)

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
	@$(REBAR) ct

eunit:
	@$(REBAR) eunit

test: eunit

check: build eunit

%.beam: %.erl
	erlc -o test/ $<

.PHONY: all clean distclean depends build etap eunit check
