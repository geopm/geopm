plugin_guide_pdf_sources = doc/geopm-plugin-guide.tex \
                           # end

EXTRA_DIST += $(plugin_guide_pdf_sources) \
              doc/geopm-plugin-guide.pdf \
              # end

doc/geopm-plugin-guide.pdf: $(plugin_guide_pdf_sources)
	cd doc && pdflatex geopm-plugin-guide.tex
	cd doc && pdflatex geopm-plugin-guide.tex
	cd doc && pdflatex geopm-plugin-guide.tex

clean-local-doc:
	rm -f doc/geopm-plugin-guide.pdf doc/*.log doc/*.aux

.PHONY: clean-local-doc
