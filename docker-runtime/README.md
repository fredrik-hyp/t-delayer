# Delayer i Docker med nginx och webbkontroll

Varje runtime-container innehåller nginx, en lokal kontrolltjänst och en eller
flera riktiga `delayer`-processer. PostgreSQL kör separat och har en beständig
volym. Ingen systemd, Docker-socket eller privileged-container behövs.

## Starta

Från repots rot:

```sh
cp .env.example .env
printf '{"instances":[]}\n' > docker-runtime/instances.json
```

Sätt egna `DB_PASSWORD` och `WEB_PASSWORD` i `.env`. Skapa därefter instanser,
inputs och outputs direkt i webbgränssnittet. För förkonfigurerade instanser kan
`docker-runtime/instances.example.json` användas som startfil i stället.

För lokalt UI-test med Docker bridge:

```sh
docker compose up -d --build
```

Öppna <http://localhost:8080>. Logga in som `admin` med `WEB_PASSWORD`.
`WEB_PORT` i `.env` ändrar porten. Standardbindningen är `127.0.0.1`.

För multicast från och till det fysiska nätverket på en Linux Docker-host:

```sh
docker compose -f compose.yaml -f compose.multicast-forward.yaml up -d --build
```

Detta delar hostens nätverk med runtime-containern för multicast. En separat
nginx-container publicerar webbporten (`WEB_BIND:WEB_PORT` till port 8080) och
vidarebefordrar till runtime-containerns nginx genom en delad Unix-socket.
Webbkontrollens autentisering finns kvar i runtime-containern. Databasen
publiceras endast på `127.0.0.1:${DB_PORT:-55432}`. Kontrollera hostens
multicast-routing och NIC. Alternativet `compose.multicast.yaml` låter runtime-
nginx lyssna direkt på hostens `WEB_PORT`, utan en separat portforward. Docker Desktop är lämpligt för UI- och interntrafiktest;
verifiera fysisk multicast på Linux innan drift. Se
[Dockers dokumentation om host networking](https://docs.docker.com/engine/network/drivers/host/).

På en host med flera nätverkskort: sätt `DELAYER_MULTICAST_INTERFACE` i `.env`
till multicastkortets lokala IPv4-adress, t.ex. `192.168.42.99`. Alla instansers
inputs går då med i multicastgrupper på det kortet och alla outputs använder
det för utskick. Inställningen kräver en containeromstart. En ogiltig eller
icke-lokal adress stoppar uppstarten. Tomt värde behåller systemets routingval.
Webbport, databasanslutning och andra tjänsters routing påverkas inte.

Webbkontrollen har en integrerad inloggningssida och en knapp för utloggning.
Användaren loggar in med `WEB_USERNAME` (standard `admin`) och `WEB_PASSWORD`.
`WEB_PASSWORD_FILE` kan peka på en monterad Docker secret i stället. Lösenordet
används endast för att skapa kontot vid första start. Därefter används den
sparade PBKDF2-hashen i `web-auth`-volymen (`/var/lib/delayer/auth.json`), även
om containern återskapas. Webbläsaren får en slumpad sessionscookie med
`HttpOnly`, `SameSite=Strict` och åtta timmars giltighet. Utloggning återkallar
sessionen direkt. Efter containeromstart behöver användaren logga in på nytt.
Misslyckade lösenordsförsök begränsas per klientadress.

Välj **Change password** bredvid **Sign out** för att byta lösenord. Ange
nuvarande lösenord och det nya lösenordet två gånger (minst åtta tecken).
Bytet sparar endast en saltad hash, återkallar alla gamla sessionscookies och
ger den aktuella webbläsaren en ny session. Delayerprocesser och outputs
påverkas inte. Ta med `web-auth`-volymen i backup; `WEB_PASSWORD` skriver
inte över ett lösenord som ändrats i UI:t.

Använd HTTPS via din vanliga reverse proxy vid åtkomst över nätverk. Sätt då
`WEB_COOKIE_SECURE=1` så att sessionscookien bara skickas över HTTPS. API:t
lyssnar enbart på en Unix-socket bakom nginx; även i host network-läge öppnas
ingen extra API-port. Ingen CORS-policy öppnar API:t för andra webbplatser.
Skrivningar kräver JSON och `X-Delayer-Control: 1`, och browseranrop måste ha
samma origin som webbgränssnittet. API-fel returneras som JSON utan att utlösa
webbläsarens inbyggda inloggningsruta.

## Konfiguration och flera instanser

Välj **New instance** i UI:t för att skapa en instans med multicastinput, bufferttid
samt en till åtta outputs. Varje output har eget namn, adress, port, TTL och
`rawudp`. Sparandet startar ingen process; välj **Start instance** när
konfigurationen är klar. Kryssrutan för autostart gäller nästa containerstart.

Välj **Configure** på en stoppad instans för att ändra input, lägga till eller
ta bort outputs och ändra deras inställningar. En körande instans måste stoppas
före nätverksändringar. Uppspelningskommandon kan fortsatt användas per output
utan omstart. Befintligt inspelat buffertmaterial behålls om input ändras.

UI-inställningar sparas i PostgreSQL, inklusive medlemskap i containern och
autostart, och återläses efter containeromstart. JSON-filen är en bootstrapfil;
redan sparade UI-inställningar skrivs inte över av den.

Exempelfilen innehåller `st1` med två outputs och `st2` med en output. `autostart`
styr om respektive process startar när containern startar. En stoppad instans kan
startas från UI:t. Instansstopp stoppar både inspelning och alla dess outputs;
paus på en output låter inspelningen och andra outputs fortsätta.

För en befintlig databas kan en post innehålla enbart namn:

```json
{"instances":[{"name":"st1","autostart":true},{"name":"st2","autostart":false}]}
```

Konfiguration med `source` och `outputs` skapar en instans **bara om namnet saknas**.
Befintliga databasinställningar skrivs aldrig över. Varje instans måste ha exakt en
konfigurationsrad i `delayer`-läge, en source, en primär sink och högst sju backup
sinks. Output 0 är primär sink; output 1–7 är backup sinks sorterade på
`priority, id`, samma ordning som i C-programmet. Exempelskapandet tilldelar unika
prioriteter. JSON-listans ordning är outputordningen för nya instanser.

Instansnamn får vara högst 40 ASCII-tecken (`A-Z`, `a-z`, siffror, `_`, `.`, `-`;
första tecknet får inte vara punkt eller bindestreck). Source och nya outputs
måste ha IPv4-multicastadresser. `msbuffer` anges i millisekunder och behåller
befintlig tidsbaserad retention. Denna funktion ändrar inte arkivformatet eller
inför obegränsad lagring. Lagring monteras på `/home/video`, som också är
`systeminfo.video_path` i den nyskapade databasen.

Start- och adresskonfiguration läses vid containerstart och processstart.
Ändringar av adresser och antal outputs kräver konfigurationsändring och
processomstart. **Uppspelningsstyrning för befintliga outputs kräver inga
omstarter.** JSON-filen är bootstrap, inte en löpande konfigurationssynk.

## Styrning och status

UI:t hämtar status varannan sekund. Status visar process/PID, input, varje outputs
adress, TTL, RTP/raw UDP, bitrate, UDP-drops, tillgänglig buffert, inspelad position,
schemalagda tider, kommandokö och de senaste driftmeddelandena. Trafikvärdena är
C-programmets befintliga medelvärden. Outputkontroller väntar på inspelad data och stängs av om statistikens
tidsstämpel är äldre än tio sekunder eller statusanslutningen försvinner.

Ovanför instanslistan visas även ledigt lagringsutrymme på inspelningskatalogens
filsystem (databasens `video_path`), CPU-belastning i procent över alla systemets
CPU:er och nätverkstrafik in/ut i Mbit/s. Lagringen visar utrymme som faktiskt är
tillgängligt för skrivning, exklusive reserverade block. CPU och nätverk räknas
från skillnaden mellan två Linux `/proc`-mätningar; första uppdateringen väntar
därför på ett andra prov. Loopback räknas inte med i nätverkstrafiken. CPU gäller
hela systemet som syns i `/proc/stat`, inte enbart delayerprocesserna eller
containerns CPU-kvot. Nätverk gäller containerns synliga nätverkskort: i bridge-läge
containerns trafik och med host networking hostens trafik. Mätningarna delas
mellan anslutna webbläsare. Om ett mätvärde eller statusanslutningen saknas visas
ett streck i stället för att gamla värden fortsätter visas som aktuella.

Varje output kan oberoende:

- **Pause:** stoppa utskick och behålla uppspelningspositionen.
- **Play:** fortsätta från pauspositionen i inspelad takt.
- **Jump / schedule (jump):** söka till en inspelad tid inom bufferten och pausa där.
- **Jump / schedule (schedule):** hoppa till inspelad tid X och starta vid verkligt klockslag Y.
- **Play until:** pausa vid valt verkligt klockslag. Tom sluttid tar bort stoppet.

Detta är befintliga `jump`, `play`, `pause`, `playto`-semantiken. `playto` är ett
**verkligt klockslag**, inte en position i inspelningen. Ett vanligt `play` utan
sluttid tar också bort tidigare sluttid. Alla tider i UI:t visas och anges i UTC,
inklusive dialogernas separata datum- och klockslagsfält. API:t kräver ISO 8601 med tidszon. Databasen
och C-processernas libpq-sessioner kör UTC.

Skrivningar returnerar `202 accepted` med kommando-ID efter att raden har
committats i `delayercoms`. Det betyder köat, inte garanterat verkställt; följ
outputstatus och loggen. Kontrolltjänsten signalerar därefter just den ägda
processen med `SIGUSR1`. C-loopen dränerar kön och pollar som reserv så att täta
kommandon inte fastnar när Unix-signaler slås ihop. Jump utanför bufferten,
okända outputs och styrning av stoppade/ej redo processer nekas.

Exempel (använd HTTPS vid fjärrstyrning):

```sh
# Login uses the same JSON endpoint as the form; keep the cookie jar private.
umask 077
python3 -c 'import json,os; print(json.dumps({"username":os.environ.get("WEB_USERNAME","admin"),"password":os.environ["WEB_PASSWORD"]}))' | \
  curl -c /tmp/delayer-cookies -H 'Content-Type: application/json' \
  -H 'X-Delayer-Control: 1' --data-binary @- http://localhost:8080/api/auth/login
curl -b /tmp/delayer-cookies http://localhost:8080/api/status
curl -b /tmp/delayer-cookies \
  -H 'Content-Type: application/json' -H 'X-Delayer-Control: 1' \
  -d '{"action":"pause"}' \
  http://localhost:8080/api/instances/st1/outputs/1/commands
curl -b /tmp/delayer-cookies -H 'Content-Type: application/json' \
  -H 'X-Delayer-Control: 1' -d '{}' http://localhost:8080/api/auth/logout
```

Konfiguration: `POST /api/instances` skapar en instans och
`POST /api/instances/{namn}/configuration` ersätter instansens inställningar när
den är stoppad. Båda tar ett objekt med `name`, `source`, `outputs`, `msbuffer`
och `autostart`, samma fält som en komplett post i exempelfilen. Dubbla namn,
kolliderande outputdestinationer och felaktiga inställningar nekas.

Övriga endpoints: `POST /api/instances/{namn}/start`, `POST .../stop` med `{}`.
`GET /api/auth/session` visar om en session finns; `POST /api/auth/login` och
`POST /api/auth/logout` hanterar inloggning och utloggning. Övriga API-endpoints
kräver en giltig session. HTML, CSS och JavaScript för inloggningsvyn samt
`/healthz` är publika. HTTP Basic authentication används inte. Ett processfel visas som
`exited` med exitkod och gör containern unhealthy; processen kan startas igen i
UI:t. Oväntat processavslut startas inte automatiskt om i en loop. Docker
`restart: unless-stopped` startar containern vid containeravslut. Efter avsiktlig
containeromstart tillämpas `autostart` på nytt; manuella outputlägen sparas inte.

## Flera containrar och befintlig PostgreSQL

Sätt `DELAYER_INSTANCES_FILE` till en annan JSON-fil per stack och använd olika
Compose-projektnamn. I host network-läge måste `WEB_PORT` och `DB_PORT` vara unika
per stack. Använd unika outputdestinationer; samma multicast host:port från två
processer ger blandad trafik. En container hanterar namnen i sin bootstrapfil samt instanser som skapats för den via UI:t.

Standardkompositionen ger varje stack en egen DB- och videovolym. Om flera
runtime-containrar använder samma externa databas: ange `PGHOST`, `PGPORT`,
`PGDATABASE`, `PGUSER` och `PGPASSWORD` eller `PGPASSFILE` och montera samma
inspelningsträd på databasens `video_path`. Standardvariablerna följer
[libpq](https://www.postgresql.org/docs/current/libpq-envars.html).
Sätt ett unikt och stabilt `DELAYER_CONTROLLER_ID` för varje runtime-container
som delar databas. Standardkompositionen använder `delayer`; detta ID kopplar
UI-skapade instanser till rätt container efter omstart.
Databasens tidszon ska vara UTC. Använd samma `PGTZ=UTC` för andra kommandoklienter.

Kontrolltjänsten håller PostgreSQL advisory locks för sina instansnamn. En andra
kontrollcontainer med samma databas och instansnamn nekas innan processer startas.
Om låsanslutningen försvinner stoppas containerns processer. Fristående äldre
systemd-processer följer inte dessa lås: stoppa dem innan samma instanser flyttas
till Docker. `/run` och PID-namespace ska vara privata; montera inte hostens
`/run` och använd inte `pid: host`.

Databasimagen initierar en **ny** datavolym med repots migrationer till version 31.
Den migrerar aldrig automatiskt en befintlig databas. För en databas på version 29,
ta sedvanlig backup och tillämpa `database_29_0-30_0.sql` före användning. Den rättar
foreign keys för backup sinks och livestatistik. `NOT VALID` lämnar historiska
rader orörda och kontrollerar nya skrivningar; granska gamla inkonsekvenser innan
constraints valideras i efterhand. Tillämpa sedan `database_30_0-31_0.sql` för UI-konfigurationens persistenta
instansregister. En befintlig databas på version 30 behöver enbart den senare
migrationen. Äldre versioner behöver föregående migrationer.

## Drift och test

```sh
docker compose logs -f delayer
docker compose ps
docker compose down                 # preserves the database and recordings
python3 docker-runtime/tests/run.py  # isolerat integrationstest, bygger images
```

Integrationstestet skapar en egen stack och tar bort **sina egna** containrar och
volymer. Det kör två C-processer och tre multicastoutputs, kontrollerar RTP och
raw UDP, paus/återupptagning per output, inspelad takt, jump, schemalagd start,
wall-clock-stopp, täta kommandon, oförändrade PIDs, ID-skillnader mellan instans och
konfiguration, sessionsinloggning/utloggning och återkallade cookies, felaktig input, processstopp/start, dubbelt ägarskap,
chunkanalys/export, inaktuell statistik, återhämtning efter processfel, UI-konfiguration av inputs/outputs, skapande av nya
instanser och beständighet efter containeromstart. Fysisk NIC/multicast-routing måste testas
på driftservern.
