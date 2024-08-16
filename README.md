# WikiMapper XML to Neo4j Converter
This project takes a Wikipedia XML dump file and converts it to a format readable by Neo4j-Admin to import it into a Neo4j database.

![g52](https://github.com/user-attachments/assets/46491544-9b49-490f-9b08-098ef8c5fb87)

# Benchmarks and Testing

### Test 1: 36 hrs 49 mins
- Conducted on my Proxmox Server, inside an LCX container
- To see the code used, checkout at the git tag `benchmarkTest1`

```
---------Info---------
Page Number: 23603262
Progress: 100.000%
Time Left: 0 hrs 0 mins 0 secs
Time Taken: 36 hrs 49 mins 15 secs
Finished processing document.
Page Count: 23603280
```

### Test 2: 10 hrs 48 mins
- Conducted on my Proxmox Server, inside an LCX container again
- To see the code used, checkout at the git tag `queuelimits`
- On top of making this code parallelised, some debugging flags that were mistakenly left in the Cmake file were removed.
- The Proxmox machine that this was run on only has 4 cores so a greater will be seen on a more modern CPU.

```
---------Info---------
Page Number: 23603262
Progress: 100.000%
Time Left: 0 hrs 0 mins 0 secs
Time Taken: 10 hrs 48 mins 48 secs
Finished processing document.
Page Count: 23603280
```
