#include <cmath>

#include "Highs.h"
#include "catch.hpp"

const bool dev_run = true;
const double inf = kHighsInf;

void checkModelScaling(const HighsInt user_bound_scale,
			 const HighsInt user_cost_scale,
			 const HighsModel& unscaled_model,
			 const HighsModel& scaled_model);

void checkSolutionScaling(const HighsInt user_bound_scale,
			 const HighsInt user_cost_scale,
			 const HighsSolution& unscaled_solution,
			 const HighsSolution& scaled_solution);

TEST_CASE("user-cost-scale-after-run", "[highs_user_scale]") {
  std::string filename =
      std::string(HIGHS_DIR) + "/check/instances/adlittle.mps";
  Highs highs;
  const HighsInfo& info = highs.getInfo();
  highs.setOptionValue("output_flag", dev_run);
  highs.readModel(filename);
  highs.run();
  HighsInfo unscaled_info = info;
  HighsSolution unscaled_solution = highs.getSolution();
  HighsModel unscaled_model = highs.getModel();
  double max_primal_infeasibility = info.max_primal_infeasibility;
  double max_dual_infeasibility = info.max_dual_infeasibility;
  double sum_dual_infeasibilities = info.sum_dual_infeasibilities;
  printf("Max primal infeasibility = %g\n", max_primal_infeasibility);
  printf("Max dual infeasibility = %g\n", max_dual_infeasibility);
  printf("Sum dual infeasibility = %g\n", sum_dual_infeasibilities);
  double objective_function_value = info.objective_function_value;

  HighsInt user_bound_scale = 10;
  double user_bound_scale_value = std::pow(2, user_bound_scale);
  highs.setOptionValue("user_bound_scale", user_bound_scale);

  HighsInt user_cost_scale = 30;
  double user_cost_scale_value = std::pow(2, user_cost_scale);
  highs.setOptionValue("user_cost_scale", user_cost_scale);

  HighsModel scaled_model = highs.getModel();
  HighsSolution scaled_solution = highs.getSolution();
  checkModelScaling(user_bound_scale, user_cost_scale, unscaled_model, scaled_model);
  checkSolutionScaling(user_bound_scale, user_cost_scale, unscaled_solution, scaled_solution);

  REQUIRE(highs.getModelStatus() == HighsModelStatus::kNotset);
  REQUIRE(info.dual_solution_status == kSolutionStatusInfeasible);
  REQUIRE(info.objective_function_value ==
          user_cost_scale_value * user_bound_scale_value * objective_function_value);
  REQUIRE(info.num_dual_infeasibilities == kHighsIllegalInfeasibilityCount);
  REQUIRE(info.max_dual_infeasibility ==
          user_cost_scale_value * max_dual_infeasibility);
  REQUIRE(info.sum_dual_infeasibilities ==
          user_cost_scale_value * sum_dual_infeasibilities);
}

TEST_CASE("user-cost-scale-after-load", "[highs_user_scale]") {
  std::string filename =
      std::string(HIGHS_DIR) + "/check/instances/adlittle.mps";
  Highs highs;
  const HighsInfo& info = highs.getInfo();
  highs.setOptionValue("output_flag", dev_run);

  highs.readModel(filename);
  HighsModel unscaled_model = highs.getModel();

  HighsInt user_bound_scale = 10;
  double user_bound_scale_value = std::pow(2, user_bound_scale);
  highs.setOptionValue("user_bound_scale", user_bound_scale);

  HighsInt user_cost_scale = 30;
  double user_cost_scale_value = std::pow(2, user_cost_scale);
  highs.setOptionValue("user_cost_scale", user_cost_scale);

  highs.readModel(filename);
  HighsModel scaled_model = highs.getModel();

  checkModelScaling(user_bound_scale, user_cost_scale, unscaled_model, scaled_model);
  //  checkSolutionScaling(user_bound_scale, user_cost_scale, unscaled_solution, scaled_solution);
  highs.run();
}

TEST_CASE("user-cost-scale-in-build", "[highs_user_scale]") {
  Highs highs;
  highs.setOptionValue("output_flag", dev_run);
  const HighsLp& lp = highs.getLp();
  const HighsInfo& info = highs.getInfo();
  const HighsSolution& solution = highs.getSolution();
  const HighsInt user_cost_scale = -30;
  const double user_cost_scale_value = std::pow(2, user_cost_scale);
  const double unscaled_col0_cost = 1e14;
  highs.addVar(0, 1);
  highs.changeColCost(0, unscaled_col0_cost);

  highs.setOptionValue("user_cost_scale", user_cost_scale);
  REQUIRE(lp.col_cost_[0] == unscaled_col0_cost *  user_cost_scale_value);

  const double unscaled_col1_cost = 1e12;
  highs.addVar(0, 1);
  highs.changeColCost(1, unscaled_col1_cost);
  REQUIRE(lp.col_cost_[1] == unscaled_col1_cost *  user_cost_scale_value);
}

void checkModelScaling(const HighsInt user_bound_scale,
		       const HighsInt user_cost_scale,
		       const HighsModel& unscaled_model,
		       const HighsModel& scaled_model) {
  const double user_bound_scale_value = std::pow(2, user_bound_scale);
  const double user_cost_scale_value = std::pow(2, user_cost_scale);
  for (HighsInt iCol = 0; iCol < unscaled_model.lp_.num_col_; iCol++) {
    REQUIRE(scaled_model.lp_.col_cost_[iCol] == unscaled_model.lp_.col_cost_[iCol] * user_cost_scale_value);
    if (unscaled_model.lp_.col_lower_[iCol] > -inf) 
      REQUIRE(scaled_model.lp_.col_lower_[iCol] == unscaled_model.lp_.col_lower_[iCol] * user_bound_scale_value);
    if (unscaled_model.lp_.col_upper_[iCol] < inf) 
      REQUIRE(scaled_model.lp_.col_upper_[iCol] == unscaled_model.lp_.col_upper_[iCol] * user_bound_scale_value);
  }
  for (HighsInt iRow = 0; iRow < unscaled_model.lp_.num_row_; iRow++) {
    if (unscaled_model.lp_.row_lower_[iRow] > -inf) 
      REQUIRE(scaled_model.lp_.row_lower_[iRow] == unscaled_model.lp_.row_lower_[iRow] * user_bound_scale_value);
    if (unscaled_model.lp_.row_upper_[iRow] < inf) 
      REQUIRE(scaled_model.lp_.row_upper_[iRow] == unscaled_model.lp_.row_upper_[iRow] * user_bound_scale_value);
  }
  
}

void checkSolutionScaling(const HighsInt user_bound_scale,
			  const HighsInt user_cost_scale,
			  const HighsSolution& unscaled_solution,
			  const HighsSolution& scaled_solution) {
  const double user_bound_scale_value = std::pow(2, user_bound_scale);
  const double user_cost_scale_value = std::pow(2, user_cost_scale);
  for (HighsInt iCol = 0; iCol < HighsInt(unscaled_solution.col_value.size()); iCol++) {
    REQUIRE(scaled_solution.col_value[iCol] == unscaled_solution.col_value[iCol] * user_bound_scale_value);
    REQUIRE(scaled_solution.col_dual[iCol] == unscaled_solution.col_dual[iCol] * user_cost_scale_value);
  }
  for (HighsInt iRow = 0; iRow < HighsInt(unscaled_solution.row_value.size()); iRow++) {
    REQUIRE(scaled_solution.row_value[iRow] == unscaled_solution.row_value[iRow] * user_bound_scale_value);
    REQUIRE(scaled_solution.row_dual[iRow] == unscaled_solution.row_dual[iRow] * user_cost_scale_value);
  }
}

