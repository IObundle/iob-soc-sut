ifdef CI
FAKE_STDIN_STDOUT="FAKE_STDIN_STDOUT"
else
FAKE_STDIN_STDOUT=
endif

all: 
	@echo "CI=$(CI)"
	@echo "FAKE_STDIN_STDOUT=$(FAKE_STDIN_STDOUT)"
