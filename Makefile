.PHONY: all clean push echo

#asciidoctor-pdf -r asciidoctor-diagram -r asciidoctor-pdf-cjk-kai_gen_gothic -a pdf-style=KaiGenGothicCN $< > /dev/null 2>&1
ORG_FILES = $(shell find . -name "*.org")
ADOC_FILES = $(shell find . -name "*.adoc")
EMACS = $(shell which emacs)
ASCIIDOCTOR = $(shell which asciidoctor)

ORG_2_HTML = $(patsubst %.org, %.html, $(ORG_FILES))
ADOC_2_HTML = $(patsubst %.adoc, %.html, $(ADOC_FILES))

all: $(ORG_2_HTML) $(ADOC_2_HTML)

%.html: %.org
	@echo $(EMACS)
	@echo $<
	@$(EMACS) $< --batch -f org-html-export-to-html --kill

%.html: %.adoc
	@echo $(ASCIIDOCTOR)
	@echo $<
	@$(ASCIIDOCTOR) -r asciidoctor-diagram -a data-uri $<

pull:
	git pull

push:
	@find . -regex '.*html\~\|.*nouse.*\|.*\.cache' -exec rm -rf {} \;
	git add -A; git commit -m "--wip auto--"
	git push

clean:
	@find . -regex '.*html\|.*html\~\|.*nouse.*\|.*\.cache' -exec rm -rf {} \;

