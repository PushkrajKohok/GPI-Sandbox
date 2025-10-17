#include "store_panel.h"
#include "../platform/fs.h"
#include <imgui.h>
#include <fstream>

static std::string leaf(const std::string& p) {
  auto k=p.find_last_of("/\\"); 
  return (k==std::string::npos)? p : p.substr(k+1);
}
static std::string read_file(const std::string& p) {
  std::ifstream f(p); 
  return f? std::string((std::istreambuf_iterator<char>(f)),{}):"";
}
static std::string jget(const std::string& txt, const char* key) {
  auto k=txt.find(std::string("\"")+key+"\"");
  if(k==std::string::npos) return "";
  auto c=txt.find(":",k); auto q1=txt.find("\"",c); auto q2=txt.find("\"",q1+1); 
  return txt.substr(q1+1,q2-q1-1);
}

std::vector<PluginMeta> scan_with_metadata(const std::string& dir) {
  auto libs = fsu::list_shared_libs(dir);
  std::vector<PluginMeta> out;
  for (auto& p: libs) {
    PluginMeta m; m.path=p; m.name=leaf(p); m.version=""; m.summary="";
    auto dot = p.find_last_of('.');
    std::string base = (dot==std::string::npos)? p : p.substr(0, dot);
    auto meta_path = base + ".plugin.json";
    auto txt = read_file(meta_path);
    if (!txt.empty()) {
      auto n=jget(txt,"name"); if(!n.empty()) m.name=n;
      m.version=jget(txt,"version"); m.summary=jget(txt,"summary");
      m.icon=jget(txt,"icon");
    }
    out.push_back(std::move(m));
  }
  return out;
}

bool draw_store_panel(std::vector<PluginMeta>& metas, int& selected, bool& load_clicked) {
  bool changed=false;
  load_clicked = false;
  ImGui::SetNextWindowSize({520,360}, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Plugin Store")) {
    if (ImGui::Button("Refresh")) changed=true;
    ImGui::Separator();
    for (int i=0;i<(int)metas.size();++i) {
      ImGui::PushID(i);
      bool sel = (i==selected);
      if (ImGui::Selectable((metas[i].name + "###"+std::to_string(i)).c_str(), sel, ImGuiSelectableFlags_SpanAllColumns)) {
        selected=i; changed=true;
      }
      ImGui::TextDisabled("%s", metas[i].summary.c_str());
      if (!metas[i].version.empty()) { ImGui::SameLine(); ImGui::Text("v%s", metas[i].version.c_str()); }
      ImGui::PopID();
    }
    
    ImGui::Separator();
    if (ImGui::Button("Load Plugin") && selected >= 0) {
      load_clicked = true;
    }
  }
  ImGui::End();
  return changed;
}
