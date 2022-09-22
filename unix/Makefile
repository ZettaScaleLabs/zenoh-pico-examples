all:
	mkdir -p ./build

	gcc -O3 -Wall -DZENOH_LINUX z_scout.c -lzenohpico -o build/z_scout
	gcc -O3 -Wall -DZENOH_LINUX z_pub.c -lzenohpico -o build/z_pub
	gcc -O3 -Wall -DZENOH_LINUX z_sub.c -lzenohpico -o build/z_sub
	gcc -O3 -Wall -DZENOH_LINUX z_pull.c -lzenohpico -o build/z_pull
	gcc -O3 -Wall -DZENOH_LINUX z_get.c -lzenohpico -o build/z_get
	gcc -O3 -Wall -DZENOH_LINUX z_queryable.c -lzenohpico -o build/z_queryable

clean:
	rm -rf build
