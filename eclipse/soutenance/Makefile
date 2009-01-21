NAME = presentation

.PHONY : all clean reset run

all :
	pdflatex $(NAME).tex
	pdflatex $(NAME).tex

clean :
	rm -f $(NAME).log $(NAME).aux $(NAME).toc $(NAME).snm $(NAME).nav $(NAME).out *~


reset : clean
	rm -f $(NAME).pdf


run :
	evince $(NAME).pdf &
