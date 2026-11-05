# AGENTS.md

Instruktioner for agenter som arbetar vidare i detta repo.

## Projektkarta

Det har ar ett C-baserat MPEG-TS/RTP-over-UDP multicast-delayer-system. Huvudprogrammet tar emot en multicast-strom, skriver inkommande paket till chunkfiler pa disk och kan samtidigt spela ut samma strom live eller med delay till en eller flera multicast-sinks.

Viktigaste katalogerna:

- `pov-delayer2/`: huvudprogrammet `delayer` och dess verktyg.
- `pov-common/`: gemensam C-kod for buffertar, RUDP-header, tid, statistik, natverk och daemon-hjalpare.
- `pov-database/src/c/`: PostgreSQL-API och structs for streaminterface-konfiguration, delayer-kommandon och statistik.
- `pov-delayer2/tools/`: manuella analys- och testverktyg, bland annat `read_chunk_file` och `mc_analyser`.
- `pov-delayer2/tools/test_delayer/`: shellscript som skapar delayer-kommandon i databasen for jump/play/pause/playto-scenarier.

Borja normalt med dessa filer:

- `pov-delayer2/delayer_main.c`: init, DB-konfig, thread-start, state machine och kommandon fran `delayercoms`.
- `pov-delayer2/delayer_other.c`: multicast receiver, disk writer, disk reader, multicast sender och disk cleaner.
- `pov-delayer2/delayer.h`: delayer-structs, filstatus, state-relaterade argument.
- `pov-delayer2/delayer_common.h`: `DELAYER_STATE_*`.
- `pov-common/buffer.h` och `pov-common/buffer.c`: buffertlistor, `CHUNK_TIME_DIFF`, `PACKET_BUFFER_SIZE`.
- `pov-common/prot_rudp.h` och `pov-common/prot_rudp.c`: fil-/paket-headern som lagras fore payload.
- `pov-database/src/c/dbdelayers.h`: `JUMP`, `PLAY`, `PAUSE`, `PLAYTO` och `s_delayercoms`.
- `pov-database/src/c/dbstreaminterfaces.h`: `s_interface`, source/sink och `msbuffer`.

## Bygg och lokala kommandon

Huvudbygget sker fran `pov-delayer2/`:

```sh
./make.sh -d
```

`./make.sh` utan argument bygger release och forsoker installera. Anvand normalt `-d` under utveckling eftersom debugbygget inte installerar och ar avsett att koras foreground/non-daemoniserat.

Andra anvandbara varianter:

```sh
./make.sh -dA
./make.sh -T
```

Programmet lankas direkt med `cc` och forvantar bland annat `pthread`, `rt` och PostgreSQL/libpq headers/libraries. Pa frasch maskin kan byggfel bero pa saknade `libpq-dev`/PostgreSQL headers snarare an kodfel.

Existerande verktyg:

```sh
cd pov-delayer2/tools/read_chunk_file
./make.sh
./read_chunk_file -I /home/video/st1_chunk_123.bin -a
./read_chunk_file -I /home/video/st1_chunk_123.bin -O out.ts

cd ../mc_analyser
make
./receiver -m 239.0.0.1 -p 1234 -d
```

## Nuvarande arkitektur

Flodet i dagens `delayer`:

1. `thread_mc_receiver` tar emot UDP multicast fran `streaminterface.source[0]`.
2. Varje paket far en intern RUDP-header via `addHeader()`. Headern lagras i fil och innehaller bland annat `payloadlength`, `counter` och `sendtime` i mikrosekunder.
3. Receiver kopierar paket direkt till send-koner for paths i `DELAYER_STATE_LIVESTREAMING`.
4. Samma paket laggs i disk-kon.
5. `thread_discwriter` skriver fasta poster om `PACKET_BUFFER_SIZE` byte till chunkfiler.
6. `thread_discreader` laser chunkfiler till send-ko for paths i `DELAYER_STATE_DELAYED`.
7. `thread_mc_sender` skickar payload till multicast-sink och taktstyr via `getTimedoutPackets()`.
8. `thread_disccleaner` tar bort gamla chunkfiler enligt `streaminterface.config.msbuffer`.

Viktiga filformatdetaljer:

- Chunkfiler heter normalt `<video_path>/<interfacename>_chunk_<id>.bin`.
- Varje post ar exakt `PACKET_BUFFER_SIZE`.
- Intern headerstorlek ar `RUDP_HEADER_SIZE` = 19 byte.
- MPEG-TS payload ar normalt 1316 byte, ibland med 12 byte RTP-header.
- `current_bufelement->rtp` sattes nar forsta payloadbyten inte ar MPEG sync byte `0x47`.
- Vid utskick hoppar sendern over intern RUDP-header och kan aven strippa RTP om sink har `rawudp`.

Tid och taktning:

- `addHeader()` anvander `gettimeofday()` och sparar wall-clock-tid i mikrosekunder i `sendtime`.
- `CHUNK_TIME_DIFF` ar 10 sekunder, sa chunkar ar tidsbaserade, inte storleksbaserade.
- `taste_file()` laser forsta och sista posten i en chunk for att satta `start_time` och `end_time`.
- `find_time()` gor linjar lasning i en chunk tills `header.sendtime >= searched_time`.
- Dagens delayed playback fungerar genom att `pause_diff = now - pause_time`, sedan later `thread_mc_sender` skicka paket nar `now - packet.sendtime >= buffer_timeout`.

## Databas och runtime

Programmet hamtar runtime-konfiguration ur PostgreSQL:

- `dbfetch_video_path()` satter global `video_path`; README anger normalt `/home/video`.
- `dbfetchstreaminterfacebyname()` hamtar interfacet fran argv[1].
- `dbfetchstreaminterfaceconfigbystreaminterfaceid()` maste ge `mode == "delayer"`.
- `streaminterface.config.msbuffer` styr hur lange disk-cleanern sparar material idag.
- Source/sink hamtas via `dbfetchstreaminterfacesourcebystreaminterfaceconfigid()` och `dbfetchstreaminterfacesinkbystreaminterfaceconfigid()`.
- Backup-sinks blir extra output paths upp till `MAX_INTERFACE_PATH`.
- Runtime-kommandon hamtas fran tabellen/API:t `delayercoms` nar processen far `SIGUSR1`.
- `SIGUSR2` laser om `msbuffer`, men observera att nuvarande kod uppdaterar `streaminterface.config.msbuffer` utan att uppdatera `total_disc_buffer_time` som cleaner-traden faktiskt anvander.

## Kodstil och saker att vara forsiktig med

- Behall C99/GNU99-stilen, syslog/`LOGMSG`-monster och befintliga pthread-mutexmonster.
- Undvik stora refaktoreringar innan det finns regressionsskydd; koden ar multitradad och timingkanslig.
- Var extra forsiktig med globala variabler: `pstate`, `streaminterface`, `video_path`, `dbconn`, `mutex_stat` och buffertlistor delas mellan tradar.
- Vid andringar i koer/listor: kontrollera bade `buffer_elements` och `no_buffer_elements`.
- Ror inte `sendtime`-semantiken lattvindigt. Den anvands bade som arkivtid och for playback-taktning.
- Nuvarande fil-ID ar `uint32_t`; mycket lang arkivering kan till slut krava wrap-/namnstrategi eller annan katalogstruktur.
- Filnamnssokningen i `check_for_long_delay_files()` skannar hela `video_path` och plockar min/max id. Det kan bli dyrt med langtidsarkiv.
- `taste_file()` antar fasta record-storlekar och att sista hela posten gar att lasa. Delvis skrivna eller kraschade chunkar hanteras grovt med `CALCULATED_END_TIME`.
- `thread_disccleaner` tar bort filer med `unlink()`. Stang av, ersatt eller gor retention-konfigurerbar innan "spara obegransat" testas.
- Multicast och systemd-korning kan krava root/capabilities och korrekt NIC/multicast-routing.

## Mal for nasta funktion: langtidsarkiv och replay fran valfri tid

Onskad ny funktionalitet:

1. Kunna spara strommar pa disk utan tidsbaserad borttagning.
2. Kunna starta ett separat program som spelar upp sparad strom fran en anvandarvald tidpunkt.
3. Uppspelningen ska ske i ratt takt, baserat pa inspelade tidsstamplar.

Rekommenderad teknisk riktning:

- Separera retention fran delay-cache. Dagens `msbuffer` ar en cachetid; langtidsarkiv behover egen policy, t.ex. "never delete", max disk, max age eller extern cleanup.
- Infor ett manifest/index per stream, inte bara katalogskanning. Minsta nyttiga index: chunk-id, filnamn, start_time, end_time, first_packet_offset, last_packet_offset, status, payload/RTP-indikation.
- Lat recordern skriva atomiskt: skriv aktiv chunk som `.tmp` eller markera `IN_PROGRESS`, och publicera/markera komplett nar chunk stangs.
- Gor replayprogrammet fristaende fran live-delayerns state machine. Det bor kunna lasa arkiv/index, hitta startchunk, soka offset och skicka till angiven multicast host/port/ttl.
- Taktning for replay bor mappa inspelad tid till uppspelningstid:

```text
play_base_wall_time = user_start_time_or_now
record_base_time = first_packet.sendtime
send packet when now >= play_base_wall_time + (packet.sendtime - record_base_time)
```

- Stod bade "spela nu fran inspelad tid X" och "starta vid wall-clock Y fran inspelad tid X" om behovet finns.
- Behall raw/RTP-beteende fran `thread_mc_sender`: skicka `packet_buffer + RUDP_HEADER_SIZE + strip_rtp` och langd `payloadlength - strip_rtp`.
- Fundera pa om replay ska kunna spela snabbare/langsammare, loopa, stoppa vid sluttid och rapportera drops/statistik. Bygg inte in det i forsta versionen om det inte kravs.

## Teststrategi for kommande arbete

Minsta verifiering vid forandringar:

- Bygg `pov-delayer2` med `./make.sh -d`.
- Bygg och kor `pov-delayer2/tools/read_chunk_file` pa en representativ chunk.
- Kontrollera att `read_chunk_file -a` visar monotont okande `hdr.sendtime` och rimliga `dT`.
- Exportera en chunk till `.ts` med `read_chunk_file -O` och verifiera med ett externt MPEG-TS-verktyg om tillgangligt.
- Testa replay mot lokal multicast-mottagare, t.ex. `mc_analyser/receiver`, och kontrollera payloadlangd, RTP/MPEG-detektion och takt.
- Testa start fran:
  - exakt forsta paketet i en chunk,
  - mitt i en chunk,
  - mellan tva chunkar,
  - tid fore arkivets start,
  - tid efter arkivets slut,
  - korrupt eller delvis skriven chunk.

## Kanda tekniska risker

- Linjar fil- och tidssokning racker for kort delay men inte for stora arkiv.
- Wall-clock-baserade `sendtime` fran `gettimeofday()` kan hoppa om systemklockan andras. Langtidsarkiv/replay bor dokumentera detta eller byta till tydligare tidsmodell for nya inspelningar.
- Dagens cleaner, statistik och state machine antar en begransad cache. Att bara hoja `msbuffer` till ett stort varde ar inte samma sak som robust arkivering.
- Diskfel stoppar processen. For langtidsinspelning behovs battre diskfyllnads-/IO-felhantering.
- Ingen tydlig metadatafil beskriver inspelningar, version av filformat eller om RTP finns. Nytt arkivformat bor vara bakatkompatibelt med befintliga chunkar.
