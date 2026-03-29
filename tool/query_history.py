import sqlite3, os

path = r'C:\Users\rinry\Tool\edge_history.db'
conn = sqlite3.connect(path)
cursor = conn.cursor()

# Search for nuvoton / numicro / nu-link related URLs
print('=== Browser History: nuvoton/numicro ===')
rows = cursor.execute("""
    SELECT urls.url, urls.last_visit_time 
    FROM urls 
    WHERE urls.url LIKE '%nuvoton%' OR urls.url LIKE '%numicro%' OR urls.url LIKE '%nu-link%' OR urls.url LIKE '%NuLink%' OR urls.url LIKE '%nu_link%'
    ORDER BY urls.last_visit_time DESC
    LIMIT 30
""").fetchall()
for r in rows:
    print(f"  {r[1]}: {r[0][:100]}")

print('\n=== Browser History: nuvoton.com ===')
rows2 = cursor.execute("""
    SELECT urls.url, urls.last_visit_time
    FROM urls
    WHERE urls.url LIKE '%nuvoton%'
    ORDER BY urls.last_visit_time DESC
    LIMIT 20
""").fetchall()
for r in rows2:
    print(f"  {r[0][:120]}")

conn.close()
