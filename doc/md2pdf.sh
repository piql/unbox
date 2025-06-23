md2pdf() {
    npx marked $1.md -o $1.html
    wkhtmltopdf --enable-local-file-access $1.html $2.pdf
    rm $1.html
}

cat SIMPLE.md DETAILED.md > tmp.md
md2pdf tmp data_decoding_description
rm tmp.md
