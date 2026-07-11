from pathlib import Path
import re
import shutil
import sys

root = Path(sys.argv[1]).resolve()
project_dir = root / "build" / "windows"
source_dir = root / "source" / "plugins" / "PackItLEDPattern"
source_dir.mkdir(parents=True, exist_ok=True)

package_root = Path(__file__).resolve().parent
shutil.copy2(package_root / "plugin" / "PackItLEDPattern.cpp", source_dir / "PackItLEDPattern.cpp")
shutil.copy2(package_root / "plugin" / "PackItLEDPattern.h", source_dir / "PackItLEDPattern.h")
shutil.copy2(package_root / "FFGLPlugins.def", project_dir / "FFGLPlugins.def")

src = (project_dir / "Gradient.vcxproj").read_text(encoding="utf-8-sig")
src = src.replace("source\\plugins\\Gradients\\FFGLGradients.cpp", "source\\plugins\\PackItLEDPattern\\PackItLEDPattern.cpp")
src = src.replace("source\\plugins\\Gradients\\FFGLGradients.h", "source\\plugins\\PackItLEDPattern\\PackItLEDPattern.h")
src = src.replace("<PlatformToolset>v142</PlatformToolset>", "<PlatformToolset>v143</PlatformToolset>")
src = re.sub(r"<WindowsTargetPlatformVersion>[^<]+</WindowsTargetPlatformVersion>", "<WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>", src)
src = src.replace("{6858F0CB-DE74-406F-B56A-CC5514CAA952}", "{7D1B63C7-21EF-43F5-A285-5C72A08F2BE0}")
(project_dir / "PackItLEDPattern.vcxproj").write_text(src, encoding="utf-8-sig")

filters_path = project_dir / "Gradient.vcxproj.filters"
if filters_path.exists():
    filters = filters_path.read_text(encoding="utf-8-sig")
    filters = filters.replace("source\\plugins\\Gradients\\FFGLGradients.cpp", "source\\plugins\\PackItLEDPattern\\PackItLEDPattern.cpp")
    filters = filters.replace("source\\plugins\\Gradients\\FFGLGradients.h", "source\\plugins\\PackItLEDPattern\\PackItLEDPattern.h")
    (project_dir / "PackItLEDPattern.vcxproj.filters").write_text(filters, encoding="utf-8-sig")

print(project_dir / "PackItLEDPattern.vcxproj")
