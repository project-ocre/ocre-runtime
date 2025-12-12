BEGIN {
    printf "/* Auto-generated file. DO NOT EDIT! */\n\n"
    printf "#include <stddef.h>\n\n"
    printf "const unsigned char ocre_mini_sample_image[] = {\n    "
    count = 0
}
{
    for(i=1; i<=NF; i++) {
        if(count > 0) printf ", "
        if(count % 12 == 0 && count > 0) printf "\n    "
        printf "0x%s", $i
        count++
    }
}
END {
    printf "\n};\n\n"
    printf "const size_t ocre_mini_sample_image_len = %d;\n", count
}
