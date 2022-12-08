DIR := ${CURDIR}
export PATH := /usr/local/opt/qt@5/bin:$(PATH)
export LDFLAGS := -L/usr/local/opt/qt@5/lib
export CPPFLAGS := -I/usr/local/opt/qt@5/include
export PKG_CONFIG_PATH := /usr/local/opt/qt@5/lib/pkgconfig

all: frontend pob 
	pushd build; \
	ninja install; \
	popd; \
	macdeployqt ${DIR}/PathOfBuilding.app; \
	cp ${DIR}/Info.plist.sh ${DIR}/PathOfBuilding.app/Contents/Info.plist; \
	echo 'Finished'

pob: load_pob luacurl frontend
	rm -rf PathOfBuildingBuild; \
	cp -rf PathOfBuilding PathOfBuildingBuild; \
	pushd PathOfBuildingBuild; \
	unzip runtime-win32.zip lua/xml.lua lua/base64.lua lua/sha1.lua; \
	mv lua/*.lua .; \
	rmdir lua; \
	cp ../lcurl.so .; \
	mv src/* .; \
	rmdir src; \
	popd

frontend: 
	meson -Dbuildtype=release --prefix=${DIR}/PathOfBuilding.app --bindir=Contents/MacOS build

load_pob:
	git clone https://github.com/PathOfBuildingCommunity/PathOfBuilding.git; \
	pushd PathOfBuilding; \
	git add . && git fetch && git reset --hard origin/dev; \
	popd

# The sed below ensures that we only replace `lua` with `luajit` once
luacurl:
	git clone --depth 1 https://github.com/Lua-cURL/Lua-cURLv3.git; \
	pushd Lua-cURLv3; \
	sed -i '' 's/\?= lua$$/\?= luajit/' Makefile; \
	sed -i '' 's@shell .* --libs libcurl@shell PKG_CONFIG_PATH=\$$\$$(brew --prefix --installed curl)/lib/pkgconfig \$$(PKG_CONFIG) --libs libcurl@' Makefile; \
	make; \
	mv lcurl.so ../lcurl.so; \
	popd

# curl is used since mesonInstaller.sh copies over the shared library dylib
tools:
	brew install qt@5 luajit zlib meson curl

# We don't usually modify the PathOfBuilding directory, so there's rarely a
# need to delete it. We separate it out to a separate task.
fullyclean: clean
	rm -rf PathOfBuilding

clean:
	rm -rf PathOfBuildingBuild PathOfBuilding.app Lua-cURLv3 lcurl.so build
