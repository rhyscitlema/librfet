
Main library for the Rhyscitlema MFET Calculator - libmfet.

To compile execute:
    gcc -c \
    -I rhyscitlema/read_write_image_file \
    -I rhyscitlema/_standard \
    -I rhyscitlema/mfet_calculator \
    -o rhyscitlema/mfet_calculator/mfet_calculator.o \
       rhyscitlema/mfet_calculator/mfet_calculator.c
