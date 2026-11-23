// I-0101 (STUDY §1, Iudex Gundyr's death), I-0113 tranche 10d: a pose update with a non-finite row publishes a version whose
// palettes keep that row (16000 has no finiteness test; the GPU drops the triangles it places), so the draw binds and the rest
// of the model is drawn, no refusal; the composed cloth pose too.
#include "../content/model_pose.h"
#include "../content/model_pose_binding.h"
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}}
int main(){try{
    using namespace ds3content;
    constexpr uint32_t count=3;
    std::vector<ModelPoseRow> rows(count);
    for(auto& row:rows){row={};row[0]=row[5]=row[10]=1.f;}
    ModelPoseOwner owner(count);
    const auto update=[&](std::vector<ModelPoseRow>& current){ModelPoseInputs inputs;inputs.row_count=count;inputs.gated=true;
        inputs.reference=ModelPoseReference::Null;inputs.gate_available=true;inputs.current=current;return owner.Update(inputs);};
    const auto finite=update(rows);
    Require(finite && finite->current.Available(),"a finite pose is Ready");
    Require(SelectOrdinaryModelPoseBinding(*finite,false,false).kind==ModelPoseBindingKind::Current,"a finite pose binds");
    rows[1][7]=std::numeric_limits<float>::quiet_NaN();
    const auto broken=update(rows);
    Require(broken!=nullptr,"a non-finite pose still publishes its version (no refusal)");
    Require(broken->current.Available() && broken->current.rows.size()==count && std::isnan(broken->current.rows[1][7]),"its palette is Ready and keeps the row");
    Require(SelectOrdinaryModelPoseBinding(*broken,false,false).kind==ModelPoseBindingKind::Current,"its draw binds");
    rows[1][7]=std::numeric_limits<float>::infinity();
    Require(update(rows)->current.Available(),"an infinite row too");
    rows[1][7]=0.f;
    const auto again=update(rows);
    Require(again->generation>broken->generation && again->current.Available(),"the next finite update is Ready");
    Require(SelectOrdinaryModelPoseBinding(*again,false,false).kind==ModelPoseBindingKind::Current,"and binds");
    // The Gundyr case (STUDY §1.2: a cloth-bearing model, composed with gate rows and reference rows): a NaN in the source rows
    // or in the gate rows is carried into the composed palette and the cloth pair, which stay bound.
    ModelPoseOwner cloth(count);
    std::vector<ModelPoseRow> gates=rows,references=rows;
    const auto compose=[&](std::vector<ModelPoseRow>& current,std::vector<ModelPoseRow>& gate){ModelPoseInputs inputs;inputs.row_count=count;
        inputs.gated=true;inputs.compose=true;inputs.auxiliary_present=true;inputs.gate_available=true;inputs.current=current;inputs.gate_rows=gate;
        inputs.reference=ModelPoseReference::Present;inputs.reference_rows=references;return cloth.Update(inputs);};
    const auto composed=compose(rows,gates);
    Require(composed && composed->current.Available() && composed->auxiliary.Available() && composed->cancel.Available(),"a finite cloth pose composes");
    rows[2][3]=std::numeric_limits<float>::quiet_NaN();
    const auto source_nan=compose(rows,gates);
    Require(source_nan && source_nan->current.Available() && source_nan->auxiliary.Available() && source_nan->current.rows.size()==count &&
        std::isnan(source_nan->auxiliary.rows[2][3]),"a NaN source row is carried into the composed and auxiliary palettes, no refusal");
    Require(SelectOrdinaryModelPoseBinding(*source_nan,true,false).kind==ModelPoseBindingKind::Auxiliary,"the cloth draw binds its auxiliary palette");
    rows[2][3]=0.f;gates[0][0]=std::numeric_limits<float>::quiet_NaN();
    const auto gate_nan=compose(rows,gates);
    Require(gate_nan && gate_nan->current.Available() && gate_nan->cancel.Available(),"a NaN gate row keeps the composed palette and the cancel");
    {bool carried{};for(const auto& row:gate_nan->cancel.rows)for(const auto value:row)carried=carried || std::isnan(value);
        Require(carried,"and the cancel carries it");}
    // A singular reference (a zero row: 179C0's inverse divides by a zero determinant) keeps its non-finite auxiliary palette Ready.
    gates[0][0]=1.f;const auto kept=references[1];references[1]={};
    const auto singular=compose(rows,gates);
    Require(singular && singular->auxiliary.Available() && singular->auxiliary.rows.size()==count,"a singular reference keeps the auxiliary palette");
    references[1]=kept;
    gates[0][0]=1.f;
    Require(compose(rows,gates)->current.Available(),"the next finite cloth update composes");
    // I-0113 gap 14: a model particle's palettes by 64B30's rule. A swaps x and y, G translates by (1, 2, 3): A x G (row r of A over
    // G's rows) puts G's x translation in row 1 and y's in row 0; G x A would leave the translation (1, 2, 3).
    {const ModelPoseRow swap{0,1,0,0, 1,0,0,0, 0,0,1,0},shift{1,0,0,1, 0,1,0,2, 0,0,1,3},lift{1,0,0,0, 0,1,0,0, 0,0,1,9};
        const std::vector<ModelPoseRow> a{swap,lift},g{shift,shift};const char* reason{};
        std::vector<EffectMeshPalette> out;std::vector<ModelPoseRow> full;
        std::vector<EffectModelMesh> meshes(2);meshes[1].dynamic=true;
        Require(ProduceEffectModelPalettes(true,a,g,meshes,out,full,reason),"full rows: produced");
        Require(out[0].kind==EffectMeshPaletteKind::Given && out[1].kind==EffectMeshPaletteKind::Composed && full.size()==2,"full rows: a rigid mesh the rows given, a Dynamic one composed");
        Require(full[0][3]==2 && full[0][7]==1 && full[0][11]==3 && full[1][11]==12,"full rows: A x G in 17CE0's order");
        meshes[0].default_bone=1;meshes[1].bones={1,0,-1,0};
        Require(ProduceEffectModelPalettes(false,a,g,meshes,out,full,reason) && full.empty(),"its own rows: produced");
        Require(out[0].kind==EffectMeshPaletteKind::Own && out[0].rows.size()==1 && out[0].rows[0]==lift,"its own rows: a rigid mesh with no list, its default bone");
        Require(out[1].rows.size()==2 && out[1].rows[0][11]==12 && out[1].rows[1][3]==2,"its own rows: the list composed, up to its negative entry");
        meshes[0].default_bone=2;Require(!ProduceEffectModelPalettes(false,a,g,meshes,out,full,reason) && std::string(reason)=="effect_model_bone_out_of_range","a default bone past A refused");
        meshes[0].default_bone=0;Require(!ProduceEffectModelPalettes(false,a,{},meshes,out,full,reason) && std::string(reason)=="effect_model_bind_rows_unreadable","a Dynamic mesh without G refused");
        // The reviews: a mesh not Dynamic with a list keeps its rows as given; a Dynamic mesh with an empty list has no row.
        meshes[0].bones={1,0};meshes[1].bones={};
        Require(ProduceEffectModelPalettes(false,a,g,meshes,out,full,reason) && out[0].rows.size()==2 && out[0].rows[0]==lift && out[0].rows[1]==swap,"its own rows: a rigid list gathered, not composed");
        Require(out[1].kind==EffectMeshPaletteKind::Own && out[1].rows.empty(),"its own rows: a Dynamic mesh with no list, no row");}
    std::printf("PASS model pose: a NaN or infinite row is kept in a Ready palette that binds (the GPU drops its triangles, no refusal), a finite update binds; the composed cloth pose too; a model particle's mesh palettes by 64B30's rule\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL model pose: %s\n",e.what());return 1;}}
