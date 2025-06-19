md2pdf() {
    npx marked $1.md -o $1.html
    wkhtmltopdf --enable-local-file-access $1.html $1.pdf
    rm $1.html
}

md2pdf SIMPLE
md2pdf DETAILED
