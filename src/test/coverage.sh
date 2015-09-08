for fname in *.gcda;
do
    gcov $fname
done
