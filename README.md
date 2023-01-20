# zip_split
Break large zip or 7z archives and recompress all files one by one into their own zip

It uses and embed  moor libarchive wrapper https://github.com/castedmo/moor to ease the 7z reading and poco zip [https://pocoproject.org/index.html] to zip files.
The ouput zipping is multihreaded via my favourites TBB library [https://github.com/oneapi-src/oneTBB].

It creates subdirectories every 1000 files zipped

Usage 
zip_split -i <Dir_with_bigzips> -o <Dir_to_store_res>

I used it to breakdown the ABC datasetv[https://deep-geometry.github.io/abc-dataset/] into many (more than a million ) smaller individual zip files. 
