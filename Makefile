REBAR := $(shell which rebar3 2>/dev/null || echo ./rebar3)
REBAR_URL := https://s3.amazonaws.com/rebar3/rebar3

.PHONY: clean compile tests c-tests c-coverage c-coverage-check \
	c-format c-format-check \
	benchmark benchmark-quick benchmark-update-readme

all: compile

compile: $(REBAR)
	$(REBAR) compile

tests: $(REBAR)
	$(REBAR) eunit

c-tests:
	$(MAKE) -C c_src_tests test

c-coverage:
	$(MAKE) -C c_src_tests coverage

c-coverage-check:
	$(MAKE) -C c_src_tests coverage-check

c-format:
	./scripts/c_format.sh fix

c-format-check:
	./scripts/c_format.sh check

clean: $(REBAR)
	$(REBAR) clean
	$(MAKE) -C c_src_tests clean

benchmark:
	$(MAKE) -C benchmarks benchmarks

benchmark-quick:
	$(MAKE) -C benchmarks benchmarks-quick

benchmark-update-readme:
	$(MAKE) -C benchmarks update-readme

./rebar3:
	erl -noshell -s inets start -s ssl start \
		-eval '{ok, saved_to_file} = httpc:request(get, {"$(REBAR_URL)", []}, [], [{stream, "./rebar3"}])' \
		-s inets stop -s init stop
	chmod +x ./rebar3
