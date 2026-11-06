from pathlib import Path
import re


def on_config(config):
    version = (Path(__file__).resolve().parents[1] / "VERSION").read_text(
        encoding="ascii"
    ).strip()
    if re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version) is None:
        raise ValueError("VERSION must contain a major.minor.patch version")
    config["extra"]["libvnascpi_version"] = version
    return config


def on_page_markdown(markdown, *, config, **kwargs):
    return markdown.replace("{{ libvnascpi_version }}", config["extra"]["libvnascpi_version"])
