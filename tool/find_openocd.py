import urllib.request, json

repos = [
    'https://api.github.com/repos/openocd-build/openocd-build/releases',
    'https://api.github.com/repos/xpack-dev-tools/xpack-openocd/releases',
]
for repo in repos:
    try:
        req = urllib.request.Request(repo, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=10) as r:
            data = json.loads(r.read())
            if data:
                print('Repo:', repo)
                print('  Latest:', data[0]['tag_name'])
                for a in data[0].get('assets', []):
                    name = a['name'].lower()
                    if 'win64' in name or 'windows' in name:
                        if 'zip' in name or '7z' in name or 'tar' in name:
                            print('  Asset:', a['name'])
                            print('    URL:', a['browser_download_url'][:100])
    except Exception as e:
        print('Error', repo, ':', e)
