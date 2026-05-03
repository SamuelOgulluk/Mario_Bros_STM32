# Deployment GitHub Pages

## Setup

```bash
cd docs
pip install -r requirements.txt
mkdocs serve  # local preview
```

## Build

```bash
cd docs
mkdocs build  # crée dossier site/
```

## CI/CD Automatique

Créez `.github/workflows/docs-deploy.yml` (voir template dans repo).

Déclenche automatiquement sur push vers main/master.

## Configuration GitHub

Settings → Pages : Source = gh-pages branch, root folder.

Accès : https://username.github.io/projetDemo2026/

## mkdocs.yml

Mettez à jour :
- site_url
- repo_url
- site_name
- nav (navigation)

Voir docs officielles : https://www.mkdocs.org/
