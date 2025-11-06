#!/bin/bash

# LaTeX Verification Script
echo "=========================================="
echo "LaTeX File Verification"
echo "=========================================="
echo ""

FILE="CEP_REPORT.tex"

if [ ! -f "$FILE" ]; then
    echo "❌ Error: $FILE not found"
    exit 1
fi

echo "Checking $FILE..."
echo ""

# Check for basic structure
echo "Structure Check:"
if grep -q "\\documentclass" "$FILE"; then
    echo "  ✓ Document class found"
else
    echo "  ✗ Document class missing"
fi

if grep -q "begin{document}" "$FILE"; then
    echo "  ✓ Begin document found"
else
    echo "  ✗ Begin document missing"
fi

if grep -q "end{document}" "$FILE"; then
    echo "  ✓ End document found"
else
    echo "  ✗ End document missing"
fi

# Count environments
BEGIN_COUNT=$(grep -c "\\\\begin{" "$FILE")
END_COUNT=$(grep -c "\\\\end{" "$FILE")
echo "  ✓ Begin environments: $BEGIN_COUNT"
echo "  ✓ End environments: $END_COUNT"

if [ "$BEGIN_COUNT" -eq "$END_COUNT" ]; then
    echo "  ✓ All environments properly matched"
else
    echo "  ✗ Mismatched environments!"
fi

# Check for logo issue
if grep -q "includegraphics.*logo.png" "$FILE"; then
    if grep -q "^[[:space:]]*%.*includegraphics.*logo.png" "$FILE"; then
        echo "  ✓ Logo reference is commented out (safe)"
    else
        echo "  ⚠ Logo reference active - ensure logo.png exists"
    fi
else
    echo "  ✓ No logo reference found"
fi

# Check required packages
echo ""
echo "Package Check:"
REQUIRED_PKGS=("geometry" "hyperref" "listings" "xcolor" "booktabs" "enumitem" "fancyhdr" "titlesec")
for pkg in "${REQUIRED_PKGS[@]}"; do
    if grep -q "\\usepackage.*{$pkg}" "$FILE" || grep -q "\\usepackage\[.*\]{$pkg}" "$FILE"; then
        echo "  ✓ Package $pkg included"
    else
        echo "  ✗ Package $pkg missing"
    fi
done

# Statistics
echo ""
echo "Document Statistics:"
LINES=$(wc -l < "$FILE")
CHARS=$(wc -c < "$FILE")
SECTIONS=$(grep -c "section{" "$FILE")
LISTINGS=$(grep -c "begin{lstlisting}" "$FILE")
TABLES=$(grep -c "begin{table}" "$FILE")

echo "  - Total lines: $LINES"
echo "  - Total characters: $CHARS"
echo "  - Sections: $SECTIONS"
echo "  - Code listings: $LISTINGS"
echo "  - Tables: $TABLES"

echo ""
echo "=========================================="
echo "✅ Verification complete!"
echo "=========================================="
echo ""
echo "To compile the document, run:"
echo "  pdflatex CEP_REPORT.tex"
echo "  pdflatex CEP_REPORT.tex  # Run twice for references"
echo ""
echo "Or use latexmk:"
echo "  latexmk -pdf CEP_REPORT.tex"
echo ""

