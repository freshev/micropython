import uos
import ubinascii
try:
    uos.stat('main.py')
except:
    f=open('main.py', 'w')
    f.write(ubinascii.a2b_base64('aW1wb3J0IG1pY3JvcHl0aG9uCmltcG9ydCBtYWNoaW5lCmltcG9ydCB1b3MKaW1wb3J0IG5ldHdvcmsKaW1wb3J0IHRpbWUKaW1wb3J0IHNlbGVjdAppbXBvcnQgc29ja2V0CmltcG9ydCBnYwoKZXNzaWQgPSAnQVNRVUVfQVAnCnBhc3N3b3JkID0gJ1ZlcnlTdHJvbmdQYXNzd29yZCEnCnVhcnRfcG9ydCA9IDAKdWFydF9zcGVlZCA9IDExNTIwMAp0Y3BfcG9ydCA9IDgwMjMKUkVTRVQgPSBGYWxzZQpERUJVRyA9IEZhbHNlCgpkZWYgZGVidWcoKmFyZ3MsICoqa3dhcmdzKToKICAgIGlmIERFQlVHOgogICAgICAgIHByaW50KCphcmdzLCAqKmt3YXJncykKCmNsYXNzIHNlcnZlcjoKICAgIGRlZiByZXN0b3JlX3JlcGwoc2VsZiwgcyk6CiAgICAgICAgdW9zLmR1cHRlcm0oTm9uZSwxKQogICAgICAgIHVvcy5kdXB0ZXJtKG1hY2hpbmUuVUFSVCgwLCAxMTUyMDApLCAxKSAjIHJlc3RvcmUgUkVQTAogICAgICAgIGRlYnVnKHMpCgogICAgZGVmIHJ1bihzZWxmLCBhZGRyZXNzKToKICAgICAgICBkZWJ1ZygnQWRkcmVzczogJyArIGFkZHJlc3MgKyAnXHJcbicpCgogICAgICAgIHRyeToKICAgICAgICAgICAgdGNwID0gc29ja2V0LnNvY2tldCgpCiAgICAgICAgICAgIHRjcC5zZXRzb2Nrb3B0KHNvY2tldC5TT0xfU09DS0VULCBzb2NrZXQuU09fUkVVU0VBRERSLCAxKQogICAgICAgICAgICB0Y3AuY29ubmVjdCgoYWRkcmVzcywgdGNwX3BvcnQpKQoKICAgICAgICAgICAgdWFydCA9IHVvcy5kdXB0ZXJtKE5vbmUsIDEpICMgZGlzYWJsZSBSRVBMCiAgICAgICAgICAgIHVhcnQgPSBtYWNoaW5lLlVBUlQodWFydF9wb3J0KQogICAgICAgICAgICB1YXJ0LmluaXQoYmF1ZHJhdGU9dWFydF9zcGVlZCwgYml0cz04LCBwYXJpdHk9Tm9uZSwgc3RvcD0xLCByeGJ1Zj0xMDI0KSAjcnhidWYgTVVTVCBiZSBzZXQKCiAgICAgICAgICAgIHBvbGxlciA9IHNlbGVjdC5wb2xsKCkKICAgICAgICAgICAgcG9sbGVyLnJlZ2lzdGVyKHVhcnQsIHNlbGVjdC5QT0xMSU4pCiAgICAgICAgICAgIHBvbGxlci5yZWdpc3Rlcih0Y3AsIHNlbGVjdC5QT0xMSU4pCgogICAgICAgICAgICB0Y3Aud3JpdGUoJ1xyXG5TdGFydCBsb2dnaW5nLi4uXHJcbicpCiAgICAgICAgICAgIG1pY3JvcHl0aG9uLmtiZF9pbnRyKC0xKQoKICAgICAgICAgICAgdHJ5OgogICAgICAgICAgICAgICAgd2hpbGUoVHJ1ZSk6CiAgICAgICAgICAgICAgICAgICAgcG9sbCA9IHBvbGxlci5wb2xsKDApCiAgICAgICAgICAgICAgICAgICAgaWYobGVuKHBvbGwpID4gMCk6CiAgICAgICAgICAgICAgICAgICAgICAgIGlmIGlzaW5zdGFuY2UocG9sbFswXVswXSwgdHlwZSh1YXJ0KSk6CiAgICAgICAgICAgICAgICAgICAgICAgICAgICBpbmJ5dGVzID0gdWFydC5yZWFkKCkKICAgICAgICAgICAgICAgICAgICAgICAgICAgIHRjcC53cml0ZShpbmJ5dGVzKQoKICAgICAgICAgICAgICAgICAgICAgICAgaWYgaXNpbnN0YW5jZShwb2xsWzBdWzBdLCB0eXBlKHRjcCkpOgogICAgICAgICAgICAgICAgICAgICAgICAgICAgaW5ieXRlcyA9IHRjcC5yZWN2KDEwMjQpCiAgICAgICAgICAgICAgICAgICAgICAgICAgICB1YXJ0LndyaXRlKGluYnl0ZXMpCgogICAgICAgICAgICAgICAgICAgIHRpbWUuc2xlZXBfbXMoMSkKCiAgICAgICAgICAgIGV4Y2VwdCBLZXlib2FyZEludGVycnVwdDoKICAgICAgICAgICAgICAgIHNlbGYucmVzdG9yZV9yZXBsKCdDdHJsLUMgcHJlc3NlZC4gRXhpdGluZycpCiAgICAgICAgICAgICAgICByYWlzZSBLZXlib2FyZEludGVycnVwdAogICAgICAgICAgICBleGNlcHQgRXhjZXB0aW9uOgogICAgICAgICAgICAgICAgc2VsZi5yZXN0b3JlX3JlcGwoJ1BlZXIgZGlzY29ubmVjdGVkJykKICAgICAgICAgICAgZmluYWxseToKICAgICAgICAgICAgICAgIG1pY3JvcHl0aG9uLmtiZF9pbnRyKDMpCiAgICAgICAgICAgICAgICBwb2xsZXIudW5yZWdpc3Rlcih1YXJ0KQogICAgICAgICAgICAgICAgcG9sbGVyLnVucmVnaXN0ZXIodGNwKQogICAgICAgICAgICAgICAgdGNwLmNsb3NlKCkKICAgICAgICAgICAgICAgIHNlbGYucmVzdG9yZV9yZXBsKCdJZGxlIHN0YXRlJykKCiAgICAgICAgZXhjZXB0IEtleWJvYXJkSW50ZXJydXB0OgogICAgICAgICAgICBwYXNzCiAgICAgICAgZXhjZXB0IEV4Y2VwdGlvbjoKICAgICAgICAgICAgZGVidWcoJ0ZhaWxlZCBpbiBpbml0JykKCmFwID0gbmV0d29yay5XTEFOKG5ldHdvcmsuQVBfSUYpCmFwLmFjdGl2ZShGYWxzZSkKc3RhID0gbmV0d29yay5XTEFOKG5ldHdvcmsuU1RBX0lGKQpzdGEuYWN0aXZlKFRydWUpCgp3aGlsZShUcnVlKToKICAgIGlmIG5vdCBzdGEuaXNjb25uZWN0ZWQoKToKICAgICAgICBzdGEuYWN0aXZlKFRydWUpCiAgICAgICAgZGVidWcoJ0Nvbm5lY3RpbmcuLi4nLCBlbmQ9JycpCiAgICAgICAgc3RhLmNvbm5lY3QoZXNzaWQsIHBhc3N3b3JkKQogICAgICAgIGNvdW50ID0gMAogICAgICAgIHdoaWxlIHN0YS5zdGF0dXMoKSA9PSAxIGFuZCBjb3VudCA8IDIwOgogICAgICAgICAgdGltZS5zbGVlcF9tcyg1MDApICMgd2FpdCBmb3IgRVNQODI2NiBjb25uZWN0cwogICAgICAgICAgZGVidWcoJy4nLCBlbmQ9JycpCiAgICAgICAgICBjb3VudCA9IGNvdW50ICsgMQoKICAgICAgICBpZiBzdGEuaXNjb25uZWN0ZWQoKToKICAgICAgICAgICAgZGVidWcoJ3N1Y2Nlc3MnKQogICAgICAgICAgICB0c2VydmVyID0gc2VydmVyKCkKICAgICAgICAgICAgdHNlcnZlci5ydW4oc3RhLmlmY29uZmlnKClbMl0pCiAgICAgICAgICAgIGlmIFJFU0VUOiBtYWNoaW5lLnJlc2V0KCkKICAgICAgICBlbHNlOgogICAgICAgICAgICBkZWJ1ZygnZmFpbGVkIScpCgogICAgc3RhLmFjdGl2ZShGYWxzZSkKICAgIGdjLmNvbGxlY3QoKQogICAgZGVidWcoIkZyZWUgbWVtOiAiICsgc3RyKGdjLm1lbV9mcmVlKCkpKQo='))
    f.close()