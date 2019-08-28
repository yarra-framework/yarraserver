build-image :
	docker build . -f Dockerfile.build -t yarraserver-build

server : build-image
	docker run -w /build/Server --rm -v `pwd`/../output-14:/opt/yarra --mount source=ccache,target=/ccache -it yarraserver-build

serverctrl : build-image
	docker run -w /build/ServerCtrl --rm -v `pwd`/../output-14:/opt/yarra --mount source=ccache,target=/ccache -it yarraserver-build
