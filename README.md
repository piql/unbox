# Preliminary plan for reading

1. CLI arg #1: folder input
2. Read folder, get all frame filenames
3. Start reading frame 1
4. Unbox as control frame v7
   1. if fails, unbox as control frame v6, then try v5, then try v4, ...
   2. return first succeeding parse
   3. If all fail, repeat step 4 with the last frame, counting down to
      second-last, third-last, etc if they fail
5. Get tocs
6. For each toc:
   1. Read all frames in toc
   2. If metadata frame ID is wrong, try adjusting by reading the frame at the
      relevant offset, if ok update global offsets

      1. If offsets fail, try reversing frame ID's
      2. Repeat this logic for every frame reading
   3. Parse toc
   4. If reading or parsing fails, continue, otherwise break.
7. For each file in toc:
   1. Read all frames in file and write out to arg #2 (folder) as you read
   2. If any read or write fails, skip the file and start next file

## notes

- Cache the last read frame between files, usually files overlap, and we don't
  want to unbox the same frame twice
