all: configure compile run
S = \n[1;33m--
E = [0m

.PHONY: all

configure:
	@echo "$(S) Configuring builds $(E)"
	@mkdir -p build && cd build && cmake ..

compile:
	@echo "$(S) Compiling sources $(E)"
	@make app -C build -j8

run:
	@echo "$(S) Running the app $(E)"
	@./build/app/app

debug:
	@echo "$(S) Debugging the app $(E)"
	@lldb -f build/app/app

test:
	@echo "$(S) Compiling tests $(E)"
	@make tests -C build -j8
	@echo "$(S) Running tests $(E)"
	@./build/tests/tests

report:
	@echo "$(S) Compiling static analysis report $(E)"
	@scan-build make app -C build

clean:
	@echo "$(S) Compiling sources $(E)"
	@make clean -C build
	@echo "done!"

reset:
	@echo "$(S) Removing all build data $(E)"
	@rm -rf build
	@echo "done!"
