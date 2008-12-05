NAME = rapport

.PHONY : all dvi ps pdf clean reset run

all : dvi ps

dvi :
	latex $(NAME).tex
#if [-e $(NAME).bib]; then bibtex $(NAME); fi
	#bibtex $(NAME)
	#latex $(NAME).tex
	latex $(NAME).tex

ps :
	dvips $(NAME).dvi -o $(NAME).ps

pdf :
	dvipdf $(NAME).dvi $(NAME).pdf


clean :
	rm -f $(NAME).dvi $(NAME).log $(NAME).aux $(NAME).blg $(NAME).bbl $(NAME).toc *~ *#


reset : clean
	rm -f $(NAME).ps $(NAME).pdf


run :
	evince $(NAME).ps &
