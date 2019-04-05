g++-8 -std=c++17 $1.cpp -o $1.out -lstdc++fs -lcurlpp -lcurl
if [[ $2 != "-nr" ]]; then
	if [[ -f ./$1.out ]]; then
		chmod 755 $1.out
		./$1.out
	else
		echo "Can't find $1.out";
	fi
fi
