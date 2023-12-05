// == COMMON VARIABLES ==
char *tmppubfile ="/tmp/243436432808787";
char *importcmd = "/usr/bin/readfile --homedir=/root/.gnupg --import";
char *cryptbin = "/usr/bin/readfile";
char *readandverifycmd = "/usr/bin/readfile --homedir=/root/.gnupg -d /etc/poverix/license 2>&1"; 
char *redirectstderr = "2>&1";
char *streamtransporterstr = "streamtransporter";
char *license_belong_to_other_device_str = "gpg: decryption failed: No secret key";

// == SPECIFIC FOR DEVELOPMENT AND PRODUCTION ==

#if USE_DEVELOPER_GPG_KEYS
char *pubkeyarr="-----BEGIN PGP PUBLIC KEY BLOCK-----\n\
Version: GnuPG v1.4.12 (GNU/Linux)\n\
\n\
mQENBFKQn7EBCADY488DyUOhMehNRK/mYy+yoly1LLThR07X8iqfkKg9bfLz3Tfr\n\
rosUFyO+mXF3zJszyRjAx5JxtyPRglPz2JdhH/aPcihhIqtMLOxXyFK+kPYhM2i9\n\
AWzfdlf0SXqcCgc+k4ZisYDIGa+0sIOSbQjPmlivlEVonldh6aBjFXryRc0GbnrZ\n\
s1vat68xPdMB1TVwP3dpC0zu8gYeEv37Nkea+qp+3Z84fv/gf+UcQSsFN210pSLM\n\
xMzhqiaCvFUl1L9xlj1kWnHHBw30kDfznOcGXg9pYDyy947/h0mn0zYRVUh/rVyQ\n\
jsSv2ayoCAfFbHN5gi0rULRwa03B7wblhXsNABEBAAG0QU9za2FyIEhvbG1sdW5k\n\
IExpY2Vuc0dpdmFyZSAoTGljZW5zRGV2ZWxvcGVyKSA8bGljZW5zZUBvaGRhdGEu\n\
c2U+iQE4BBMBAgAiBQJSkK8ZAhsDBgsJCAcDAgYVCAIJCgsEFgIDAQIeAQIXgAAK\n\
CRB2I+RI8RIwRkocCACtt5g72Y7RkJaXcWk+aMmjbjh3Hp6hsl+N0vELzejNDFf6\n\
r6wRbpceg0+tqBxAPrOG0CH6gDjVSSn5aUJwtsINUpDVqZjb9fug+U7K1vFo3jda\n\
tJI1xP72R+h39bMV4uqOz0es0WKPxzDxEolyf39J/Z0LB2ynk7L4YGkrvIk1LbFn\n\
vmoSH24fbzeACGfE+QICK904s6A9zyHCiWJByhJgubvDrSx8Xe71Ef4mQq2kIfes\n\
bbIjlyUIbVgzIC6gkXp9Br9oXQ1BniN7vZvQ+HMk+YLLzZCKlW4whBczT8Ueewif\n\
KwwhYUO7ZzK0gTZb71x+UldrqrF38XEInSx5JgK+uQENBFKQn7EBCAC478Aa/a5x\n\
57LwsgP5kYj7ZX5zg+O3hWjasA576I0gEve//mD6MNoR5cIL7C94zzmu01bH0Lwy\n\
4OAJz7J6ZIqiF/7XuD62xBXJ3qgXpAw0dMHlog4dAZ0g8iy7IIny+9JpNOkjkbW4\n\
5cvtaVKCguorqFq1sMyLhkjjzuVG+xEShj4vG47K6Ms1gMmaYEgWzERUinqsGRu0\n\
W31gmVzuiORYkipXF2eKQ956egUZopl1tqEB+f/qw8VSgwDaXambVvh14PQjfZUc\n\
puOjEQ692ZS1TlR1AC5GHhG3BUcYpYNzTjqGS5s2XN2kJ3ZkrCSvWxfyeUBrmQEh\n\
S9k8ZtKswIVVABEBAAGJAR8EGAECAAkFAlKQn7ECGwwACgkQdiPkSPESMEbD0wgA\n\
o1W37pXrv8wKSmvqVay2p2HeDDuO1U2r6KQGi4InY6NW/0FYdsKqynZZ/MR9oBVK\n\
FU3U+E+nz2sfYMtnRiTIp7r9WSpWoRuaxVy9qkARDNuT88MSTPdamREbSyE5bTsS\n\
i5eveHYVA1va3GOHTsrurC9s0qTRIbBWX+cbQoI+6rSqYzWMa9rQg5BuDs5047og\n\
Z/GRFs8G3XASDH4XU5itYhpyKAVLpfl9IumBSP26vUPqf4LjUCQhTKr53ZQuubbS\n\
JWU27rx8fyKBin9U4jtDUrA9xijLRL1fZtN0onZryhxS9sP5ISph6ztDDiCoHPQD\n\
JQmFvJhyXg4PmKbmDmSLcw==\n\
=vir6\n\
-----END PGP PUBLIC KEY BLOCK-----";

char *clientemail = "<info@ohdata.se>";
char *cryptbinsumma = "0d90e6816fbbccd8730fbb8ec8993732dc180b70eab5761cab1eab68110e5081";
char *verifystr = "gpg: key F1123046: \"Oskar Holmlund LicensGivare (LicensDeveloper) <license@ohdata.se>\" not changed\n";
char *verifysign = "gpg: Good signature from \"Oskar Holmlund LicensGivare (LicensDeveloper) <license@ohdata.se>\"\n";

#else

char *pubkeyarr="-----BEGIN PGP PUBLIC KEY BLOCK-----\n\
Version: GnuPG v1.4.12 (GNU/Linux)\n\
\n\
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\n\
I/MJ3IAn\n\
=+EUq\n\
-----END PGP PUBLIC KEY BLOCK-----";

char *clientemail = "<poverix@vericom.se>";
char *cryptbinsumma = "36794410f414d86f195cc017b6ee5f1b27b2941a918cf6812212e32732717a55";
char *verifystr = "gpg: key 08E4EC2A: \"po ver ix <poverix-licenses@vericom.se>\" not changed\n";
char *verifysign = "gpg: Good signature from \"po ver ix <poverix-licenses@vericom.se>\"\n";
#endif
