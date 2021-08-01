/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                       */
/*    This file is part of the HiGHS linear optimization suite           */
/*                                                                       */
/*    Written and engineered 2008-2021 at the University of Edinburgh    */
/*                                                                       */
/*    Available as open-source under the MIT License                     */
/*                                                                       */
/*    Authors: Julian Hall, Ivet Galabova, Qi Huangfu, Leona Gottwald    */
/*    and Michael Feldmeier                                              */
/*                                                                       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**@file lp_data/HighsLp.cpp
 * @brief
 */
#include "lp_data/HighsLp.h"

#include <cassert>

bool HighsLp::isMip() const {
  HighsInt integrality_size = this->integrality_.size();
  if (integrality_size) {
    assert(integrality_size == this->num_col_);
    for (HighsInt iCol = 0; iCol < this->num_col_; iCol++)
      if (this->integrality_[iCol] != HighsVarType::kContinuous) return true;
  }
  return false;
}

bool HighsLp::operator==(const HighsLp& lp) {
  bool equal = equalButForNames(lp);
  equal = this->row_names_ == lp.row_names_ && equal;
  equal = this->col_names_ == lp.col_names_ && equal;
  return equal;
}

bool HighsLp::equalButForNames(const HighsLp& lp) const {
  bool equal = true;
  equal = this->num_col_ == lp.num_col_ && equal;
  equal = this->num_row_ == lp.num_row_ && equal;
  equal = this->sense_ == lp.sense_ && equal;
  equal = this->offset_ == lp.offset_ && equal;
  equal = this->model_name_ == lp.model_name_ && equal;
  equal = this->col_cost_ == lp.col_cost_ && equal;
  equal = this->col_upper_ == lp.col_upper_ && equal;
  equal = this->col_lower_ == lp.col_lower_ && equal;
  equal = this->row_upper_ == lp.row_upper_ && equal;
  equal = this->row_lower_ == lp.row_lower_ && equal;
  equal = this->a_start_ == lp.a_start_ && equal;
  equal = this->a_index_ == lp.a_index_ && equal;
  equal = this->a_value_ == lp.a_value_ && equal;
  equal = this->format_ == lp.format_ && equal;

  equal = this->a_matrix_.format == lp.a_matrix_.format && equal;
  equal = this->a_matrix_.num_col == lp.a_matrix_.num_col && equal;
  equal = this->a_matrix_.num_row == lp.a_matrix_.num_row && equal;
  equal = this->a_matrix_.start == lp.a_matrix_.start && equal;
  equal = this->a_matrix_.index == lp.a_matrix_.index && equal;
  equal = this->a_matrix_.value == lp.a_matrix_.value && equal;

  equal = this->scale_.strategy == lp.scale_.strategy && equal;
  equal = this->scale_.has_scaling == lp.scale_.has_scaling && equal;
  equal = this->scale_.num_col == lp.scale_.num_col && equal;
  equal = this->scale_.num_row == lp.scale_.num_row && equal;
  equal = this->scale_.cost == lp.scale_.cost && equal;
  equal = this->scale_.col == lp.scale_.col && equal;
  equal = this->scale_.row == lp.scale_.row && equal;
  return equal;
}

double HighsLp::objectiveValue(const std::vector<double>& solution) const {
  assert((int)solution.size() >= this->num_col_);
  double objective_function_value = this->offset_;
  for (HighsInt iCol = 0; iCol < this->num_col_; iCol++)
    objective_function_value += this->col_cost_[iCol] * solution[iCol];
  return objective_function_value;
}

bool HighsLp::dimensionsAndaMatrixOk(std::string message) const {
  return dimensionsOk(message) && aMatrixOk(message);
}

bool HighsLp::dimensionsOk(std::string message) const {
  bool ok = true;
  const HighsInt num_col = this->num_col_;
  const HighsInt num_row = this->num_row_;
  ok = num_col >= 0 && ok;
  ok = num_row >= 0 && ok;
  if (!ok) {
    printf("HighsLp::dimensionsOk (%s) illegal numbers of rows or columns\n",
           message.c_str());
    return ok;
  }

  HighsInt col_cost_size = this->col_cost_.size();
  HighsInt col_lower_size = this->col_lower_.size();
  HighsInt col_upper_size = this->col_upper_.size();
  HighsInt matrix_start_size = this->a_start_.size();
  bool legal_col_cost_size = col_cost_size >= num_col;
  bool legal_col_lower_size = col_lower_size >= num_col;
  bool legal_col_upper_size = col_lower_size >= num_col;
  bool legal_matrix_start_size = matrix_start_size >= num_col + 1;
  ok = legal_col_cost_size && ok;
  ok = legal_col_lower_size && ok;
  ok = legal_col_upper_size && ok;
  ok = legal_matrix_start_size && ok;

  HighsInt row_lower_size = this->row_lower_.size();
  HighsInt row_upper_size = this->row_upper_.size();
  bool legal_row_lower_size = row_lower_size >= num_row;
  bool legal_row_upper_size = row_lower_size >= num_row;
  ok = legal_row_lower_size && ok;
  ok = legal_row_upper_size && ok;

  bool legal_a_matrix_num_col = this->a_matrix_.num_col == num_col;
  bool legal_a_matrix_num_row = this->a_matrix_.num_row == num_row;
  ok = legal_a_matrix_num_col && ok;
  ok = legal_a_matrix_num_row && ok;

  HighsInt a_matrix_start_size = this->a_matrix_.start.size();
  bool legal_a_matrix_start_size = false;
  // Don't expect the matrix_start_size or format to be legal if there
  // are no columns
  if (num_col > 0) {
    legal_a_matrix_start_size = a_matrix_start_size >= num_col + 1;
    ok = legal_a_matrix_start_size && ok;
    HighsInt a_matrix_format = (HighsInt)this->a_matrix_.format;
    bool legal_a_matrix_format = a_matrix_format > 0;
    ok = legal_a_matrix_format && ok;
  }
  if (a_matrix_start_size > 0) {
    // Check whether the first start is zero
    ok = !this->a_matrix_.start[0] && ok;
  }
  HighsInt num_nz = 0;
  if (legal_matrix_start_size) num_nz = this->a_matrix_.start[num_col];
  bool legal_num_nz = num_nz >= 0;
  if (!legal_num_nz) {
    ok = false;
  } else {
    HighsInt a_matrix_index_size = this->a_matrix_.index.size();
    HighsInt a_matrix_value_size = this->a_matrix_.value.size();
    bool legal_a_matrix_index_size = a_matrix_index_size >= num_nz;
    bool legal_a_matrix_value_size = a_matrix_value_size >= num_nz;

    ok = legal_a_matrix_index_size && ok;
    ok = legal_a_matrix_value_size && ok;
  }

  HighsInt scale_strategy = (HighsInt)this->scale_.strategy;
  bool legal_scale_strategy = scale_strategy >= 0;
  ok = legal_scale_strategy && ok;
  if (scale_strategy) {
    bool legal_scale_num_col = this->scale_.num_col == num_col;
    ok = legal_scale_num_col && ok;
    bool legal_scale_num_row = this->scale_.num_row == num_row;
    ok = legal_scale_num_row && ok;
    HighsInt scale_row_size = (HighsInt)this->scale_.row.size();
    HighsInt scale_col_size = (HighsInt)this->scale_.col.size();
    bool legal_scale_row_size = scale_row_size >= num_row;
    bool legal_scale_col_size = scale_col_size >= num_col;
    ok = legal_scale_row_size && ok;
    ok = legal_scale_col_size && ok;
  }

  if (!ok) {
    printf("HighsLp::dimensionsOk (%s) not OK\n", message.c_str());
  }
  return ok;
}

bool HighsLp::aMatrixOk(std::string message) const {
  bool ok = true;
  ok = this->a_matrix_.format == this->format_ && ok;
  ok = this->a_matrix_.num_col == this->num_col_ && ok;
  ok = this->a_matrix_.num_row == this->num_row_ && ok;
  ok = this->a_matrix_.start == this->a_start_ && ok;
  ok = this->a_matrix_.index == this->a_index_ && ok;
  ok = this->a_matrix_.value == this->a_value_ && ok;
  if (!ok) {
    printf("HighsLp::aMatrixOk (%s) not OK\n", message.c_str());
  }
  return ok;
}

bool HighsLp::equalScale(std::string message, const SimplexScale& scale) const {
  bool equal = true;
  equal = this->scale_.col == scale.col && equal;
  equal = this->scale_.row == scale.row && equal;
  if (!equal) {
    printf("HighsLp::equalScale (%s) not equal\n", message.c_str());
  }
  return equal;
}

void HighsLp::clear() {
  this->num_col_ = 0;
  this->num_row_ = 0;

  this->a_start_.clear();
  this->a_index_.clear();
  this->a_value_.clear();
  this->col_cost_.clear();
  this->col_lower_.clear();
  this->col_upper_.clear();
  this->row_lower_.clear();
  this->row_upper_.clear();

  this->a_matrix_.start.clear();
  this->a_matrix_.index.clear();
  this->a_matrix_.value.clear();

  this->sense_ = ObjSense::kMinimize;
  this->offset_ = 0;
  this->format_ = MatrixFormat::kNone;

  this->model_name_ = "";

  this->col_names_.clear();
  this->row_names_.clear();

  this->integrality_.clear();

  this->scale_.strategy = kSimplexScaleStrategyOff;
  this->scale_.has_scaling = false;
  this->scale_.num_col = 0;
  this->scale_.num_row = 0;
  this->scale_.cost = 1.0;
  this->scale_.col.clear();
  this->scale_.row.clear();
}
