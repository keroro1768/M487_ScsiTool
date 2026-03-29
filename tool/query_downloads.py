import sqlite3, os

path = r'C:\Users\rinry\Tool\edge_history.db'
if not os.path.exists(path):
    print('File not found')
    exit()

conn = sqlite3.connect(path)
cursor = conn.cursor()

# Check downloads table
print('=== Downloads ===')
rows = cursor.execute("SELECT id, current_path, target_path, received_bytes, total_bytes, state, start_time FROM downloads ORDER BY id DESC LIMIT 20").fetchall()
print(f'Found {len(rows)} download records')
for r in rows:
    print(f"  [{r[0]}] path={r[1]} | target={r[2][:60] if r[2] else 'None'} | size={r[3]}/{r[4]} | state={r[5]}")

# Search for nuvoton/nu-link related
print('\n=== Nuvoton/NuLink Downloads ===')
rows2 = cursor.execute("SELECT d.id, d.current_path, d.target_path, d.state FROM downloads d JOIN downloads_url_chains duc ON d.id = duc.id WHERE duc.url LIKE '%nu%' OR duc.url LIKE '%link%' OR duc.url LIKE '%nuvoton%' OR duc.url LIKE '%NuMicro%' LIMIT 20").fetchall()
for r in rows2:
    print(f"  [{r[0]}] {r[1]} | {r[2][:80] if r[2] else 'None'} | state={r[3]}")

# Also show recent URLs from downloads_url_chains
print('\n=== Recent Download URLs ===')
rows3 = cursor.execute("SELECT DISTINCT duc.url, d.id, d.state FROM downloads d JOIN downloads_url_chains duc ON d.id = duc.id ORDER BY d.id DESC LIMIT 10").fetchall()
for r in rows3:
    print(f"  [{r[1]}] {r[0][:100]} | state={r[2]}")

conn.close()
