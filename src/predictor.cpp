//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Joachim Galil";
const char *studentID = "A17387237";
const char *email = "jgalil@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 15; // Number of bits used for Global History
int c_ghistoryBits = 17;
int bpType;            // Branch Prediction Type
int verbose;
int tlhistoryBits = 11;  //local history length for tournament predictor
int clhistoryBits = 13;  //local history length for custom predictor
int tghistoryBits = 13;  //global history length for tournament predictor
int pcIndexBits = 11;    //index bits for tournament predictor

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

//tournament global
uint8_t *t_global_prediction_table; //table of predictions
uint64_t t_ghistory;  //global history table

//tournament local
uint16_t *t_bht_local;
uint8_t *t_local_prediction_table;

//tournament choice
uint8_t *choice_predictor;

//custom
uint8_t *c_bht_gshare;
uint64_t c_ghistory;

//custom local
uint16_t *c_bht_local;
uint8_t *c_local_prediction_table;

//tournament choice
uint8_t *c_choice_predictor;

//TAGE
int tage_ghistoryBits = 14;  // Reduced for budget
int tage_num_tables = 5;     // Number of TAGE tables
int tage_tag_widths[] = {7, 7, 8, 9, 10};  // Tag widths for each table
int tage_table_sizes[] = {512, 1024, 2048, 4096, 8192};  // Entries per table

// TAGE data structures
uint64_t tage_ghistory;
uint8_t *tage_base_table;
uint8_t **tage_tag_tables;
uint8_t **tage_pred_counters;
uint8_t *tage_useful_bits;


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

void init_custom_gshare()
{
  int c_bht_entries = 1 << c_ghistoryBits;
  c_bht_gshare = (uint8_t *)malloc(c_bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < c_bht_entries; i++)
  {
    c_bht_gshare[i] = WN;
  }
  c_ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

uint8_t custom_gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t c_bht_entries = 1 << c_ghistoryBits;
  uint32_t pc_lower_bits = pc & (c_bht_entries - 1);
  uint32_t c_ghistory_lower_bits = c_ghistory & (c_bht_entries - 1);
  uint32_t index = pc_lower_bits ^ c_ghistory_lower_bits;
  switch (c_bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void train_custom_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t c_bht_entries = 1 << c_ghistoryBits;
  uint32_t pc_lower_bits = pc & (c_bht_entries - 1);
  uint32_t c_ghistory_lower_bits = c_ghistory & (c_bht_entries - 1);
  uint32_t index = pc_lower_bits ^ c_ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (c_bht_gshare[index])
  {
  case WN:
    c_bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    c_bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    c_bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    c_bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in c_GSHARE BHT!\n");
    break;
  }

  // Update history register
  c_ghistory = ((c_ghistory << 1) | outcome);
}

void init_tournament(){
  //global predictor
  //global history

  //branch history table
  uint32_t global_entries = 1 << tghistoryBits; //2^ghistorybits
  t_global_prediction_table = (uint8_t *)malloc(global_entries * sizeof(uint8_t));
  for(int i = 0; i < global_entries; i++){
    t_global_prediction_table[i] = WT; //initialize to wt
  }
  t_ghistory = 0;
  //local predictor
  uint32_t local_entries = 1 << pcIndexBits; //2^pcindexbits
  t_bht_local = (uint16_t *)malloc(local_entries * sizeof(uint16_t));
    for (int i = 0; i < local_entries; i++) {
      t_bht_local[i] = 0; //init lhist to 0
    }

  //local prediction table
  uint32_t pattern_entries = 1 << tlhistoryBits; //2^tlhistorybits
  t_local_prediction_table = (uint8_t *)malloc(pattern_entries * sizeof(uint8_t));
  for(int i = 0; i < pattern_entries; i++){
    t_local_prediction_table[i] = 4; //start wt
  }

  uint32_t choice_entries = 1 << tghistoryBits;
  choice_predictor = (uint8_t *)malloc(choice_entries * sizeof(uint8_t));
  for(int i = 0; i < choice_entries; i++){
    choice_predictor[i] = WN; //start with weak global
  }
  
}

void init_custom(){
  //gshare in tournament
  init_custom_gshare();

  //local predictor
  uint32_t c_local_entries = 1 << pcIndexBits; //2^pcindexbits
  c_bht_local = (uint16_t *)malloc(c_local_entries * sizeof(uint16_t));
  for (int i = 0; i < c_local_entries; i++) {
    c_bht_local[i] = 0; //init lhist to 0
  }

  //local prediction table
  uint32_t c_pattern_entries = 1 << clhistoryBits; //2^clhistorybits
  c_local_prediction_table = (uint8_t *)malloc(c_pattern_entries * sizeof(uint8_t));
  for(int i = 0; i < c_pattern_entries; i++){
    c_local_prediction_table[i] = 4; //start wt
  }

  uint32_t c_choice_entries = 1 << c_ghistoryBits;
  c_choice_predictor = (uint8_t *)malloc(c_choice_entries * sizeof(uint8_t));
  for(int i = 0; i < c_choice_entries; i++){
    c_choice_predictor[i] = WN; //start with weak global
  }
}

uint32_t custom_predict(uint32_t pc){
  //indices
  uint32_t pc_idx = pc & ((1 << pcIndexBits)-1);
  uint32_t choice_index = (pc_idx ^ c_ghistory) & ((1<<c_ghistoryBits)-1);  //gshare ghist


  //local history and prediction
  uint16_t local_history = c_bht_local[pc_idx];
  uint32_t local_pattern_idx = local_history & ((1<< clhistoryBits)-1);
  uint8_t c_local_counter = c_local_prediction_table[local_pattern_idx];
  uint32_t c_local_pred = predict_3_bit(c_local_counter);

  //gshare
  uint32_t gshare_pred = custom_gshare_predict(pc);
  //choice pred
  uint8_t choice_counter = c_choice_predictor[choice_index];
  uint32_t choice_pred = predict_2_bit(choice_counter);

  //tournament choice
  return (choice_pred == TAKEN) ? c_local_pred : gshare_pred;


}

uint32_t tournament_predict(uint32_t pc){
  //indices
  uint32_t pc_idx = pc & ((1 << pcIndexBits)-1);
  uint32_t global_idx = t_ghistory & ((1 << tghistoryBits)-1);
  uint32_t choice_index = (pc_idx ^ t_ghistory) & ((1<<tghistoryBits)-1);

  //local history and prediction
  uint16_t local_history = t_bht_local[pc_idx];
  uint32_t local_pattern_idx = local_history & ((1<< tlhistoryBits)-1);
  uint8_t local_counter = t_local_prediction_table[local_pattern_idx];
  uint32_t local_pred = predict_3_bit(local_counter);

  //global pred
  uint8_t global_counter = t_global_prediction_table[global_idx];
  uint32_t global_pred = predict_2_bit(global_counter);

  //choice pred
  uint8_t choice_counter = choice_predictor[choice_index];

  uint32_t choice_pred = predict_2_bit(choice_counter);

  //choice taken = local else global
  return (choice_pred == TAKEN) ? local_pred : global_pred;
  }

void train_tournament(uint32_t pc, uint32_t outcome){
  //indices
  uint32_t pc_idx = pc & ((1 << pcIndexBits)-1);
  uint32_t global_idx = t_ghistory & ((1 << tghistoryBits)-1);
  uint32_t choice_index = (pc_idx ^ t_ghistory) & ((1<<tghistoryBits)-1);

  //get prediction again
  uint16_t local_history = t_bht_local[pc_idx];
  uint32_t local_pattern_idx = local_history & ((1 << tlhistoryBits) - 1);
  uint8_t local_counter = t_local_prediction_table[local_pattern_idx];
  uint32_t local_pred = predict_3_bit(local_counter);
  
  uint8_t global_counter = t_global_prediction_table[global_idx];
  uint32_t global_pred = predict_2_bit(global_counter);


  //train predictors used
  train_3b_counter(&t_local_prediction_table[local_pattern_idx], outcome);

  train_2b_counter(&t_global_prediction_table[global_idx], outcome);

  //train choice
  if (local_pred != global_pred) {
    if (local_pred == outcome) {
      // Local correct
      saturating_add(&choice_predictor[choice_index], 3);
    } else if (global_pred == outcome) {
      // Global correct
      saturating_sub(&choice_predictor[choice_index], 0);
    }
  }
  //if both correct/incorrect dont train

  //local history update
  t_bht_local[pc_idx] = ((t_bht_local[pc_idx] << 1) | outcome) & ((1 << tlhistoryBits) - 1);

  //global history update
  t_ghistory = ((t_ghistory << 1) | outcome) & ((1 << tghistoryBits) - 1);
}

void train_custom(uint32_t pc, uint32_t outcome){
  //indices
  uint32_t pc_idx = pc & ((1 << pcIndexBits)-1);
  uint32_t choice_index = (pc_idx ^ c_ghistory) & ((1<<c_ghistoryBits)-1);

  //get prediction again
  uint16_t local_history = c_bht_local[pc_idx];
  uint32_t local_pattern_idx = local_history & ((1 << clhistoryBits) - 1);
  uint8_t local_counter = c_local_prediction_table[local_pattern_idx];
  uint32_t local_pred = predict_3_bit(local_counter);
  
  
  uint32_t gshare_pred = custom_gshare_predict(pc);

  //train predictors used
  train_3b_counter(&c_local_prediction_table[local_pattern_idx], outcome);

  train_custom_gshare(pc, outcome);

  //train choice
  if (local_pred != gshare_pred) {
    if (local_pred == outcome) {
      // local correct
      saturating_add(&c_choice_predictor[choice_index], 3);
    } else if (gshare_pred == outcome) {
      // gshare correct
      saturating_sub(&c_choice_predictor[choice_index], 0);
    }
  }
  //if both correct/incorrect no update

  //local history update
  c_bht_local[pc_idx] = ((c_bht_local[pc_idx] << 1) | outcome) & ((1 << clhistoryBits) - 1);
  c_ghistory = ((c_ghistory << 1) | outcome) & ((1 << c_ghistoryBits) - 1);
}

void cleanup_tournament(){
  free(t_bht_local);
  free(t_local_prediction_table);
  free(t_global_prediction_table);
  free(choice_predictor);
}

void cleanup_gshare()
{
  free(bht_gshare);
}

void cleanup_custom(){
  free(c_bht_local);
  free(c_local_prediction_table);
  free(c_choice_predictor);
  free(c_bht_gshare);
}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_tournament();
    break;
  case CUSTOM:
    init_tage();
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return tournament_predict(pc);
  case CUSTOM:
    return tage_predict(pc);
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tournament(pc, outcome);
    case CUSTOM:
      return train_tage(pc, outcome);
    default:
      break;
    }
  }
}

void train_2b_counter(uint8_t *counter, uint8_t outcome){
  if(outcome == TAKEN){
    saturating_add(counter, 3);
  }
  else{
    saturating_sub(counter, 0);
  }
}

void train_3b_counter(uint8_t *counter, uint8_t outcome)
{
  if(outcome == TAKEN){
    saturating_add(counter, 7);
  }
  else{
    saturating_sub(counter, 0);
  }
}

uint32_t predict_2_bit(uint8_t counter){
  if(counter >= 2){
    return TAKEN;
  }
  else{
    return NOTTAKEN;
  }
}

uint32_t predict_3_bit(uint8_t counter){
  if(counter > 3){
    return TAKEN;
  }
  else{
  return NOTTAKEN;
  }  
}


// TAGE functions
void init_tage() {
    int base_entries = 1 << tage_ghistoryBits;
    
    // Base table (simple gshare-like)
    tage_base_table = (uint8_t *)malloc(base_entries * sizeof(uint8_t));
    for (int i = 0; i < base_entries; i++) {
        tage_base_table[i] = WN;
    }
    
    // Allocate tag tables and prediction counters
    tage_tag_tables = (uint8_t **)malloc(tage_num_tables * sizeof(uint8_t *));
    tage_pred_counters = (uint8_t **)malloc(tage_num_tables * sizeof(uint8_t *));
    
    for (int i = 0; i < tage_num_tables; i++) {
        int table_size = tage_table_sizes[i];
        tage_tag_tables[i] = (uint8_t *)malloc(table_size * sizeof(uint8_t));
        tage_pred_counters[i] = (uint8_t *)malloc(table_size * sizeof(uint8_t));
        
        for (int j = 0; j < table_size; j++) {
            tage_tag_tables[i][j] = 0;  // Start with invalid tags
            tage_pred_counters[i][j] = WN;
        }
    }
    
    // Useful bits (1-bit saturating counter per entry)
    tage_useful_bits = (uint8_t *)malloc(tage_table_sizes[tage_num_tables-1] * sizeof(uint8_t));
    for (int i = 0; i < tage_table_sizes[tage_num_tables-1]; i++) {
        tage_useful_bits[i] = 0;
    }
    
    tage_ghistory = 0;
}

uint32_t tage_predict(uint32_t pc) {
    uint32_t base_idx = (pc ^ tage_ghistory) & ((1 << tage_ghistoryBits) - 1);
    uint32_t base_pred = predict_2_bit(tage_base_table[base_idx]);
    
    // Check tag tables from longest to shortest history
    int provider_table = -1;
    uint32_t provider_pred = base_pred;
    
    for (int i = tage_num_tables - 1; i >= 0; i--) {
        uint32_t idx = tage_compute_index(pc, i);
        uint8_t tag = tage_compute_tag(pc, i);
        
        if (tage_tag_tables[i][idx] == tag) {
            provider_table = i;
            provider_pred = predict_2_bit(tage_pred_counters[i][idx]);
            break;
        }
    }
    
    // If no tag match, use base prediction
    if (provider_table == -1) {
        return base_pred;
    }
    
    return provider_pred;
}

void train_tage(uint32_t pc, uint32_t outcome) {
    uint32_t base_idx = (pc ^ tage_ghistory) & ((1 << tage_ghistoryBits) - 1);
    
    // Find provider and alternate provider
    int provider_table = -1;
    int alt_table = -1;
    
    for (int i = tage_num_tables - 1; i >= 0; i--) {
        uint32_t idx = tage_compute_index(pc, i);
        uint8_t tag = tage_compute_tag(pc, i);
        
        if (tage_tag_tables[i][idx] == tag) {
            if (provider_table == -1) {
                provider_table = i;
            } else if (alt_table == -1) {
                alt_table = i;
                break;
            }
        }
    }
    
    // Update prediction counters
    if (provider_table != -1) {
        uint32_t provider_idx = tage_compute_index(pc, provider_table);
        train_2b_counter(&tage_pred_counters[provider_table][provider_idx], outcome);
    } else {
        // Train base predictor
        train_2b_counter(&tage_base_table[base_idx], outcome);
    }
    
    // Allocate new entries on misprediction if we have an alternate provider
    if (provider_table != -1 && predict_2_bit(tage_pred_counters[provider_table][tage_compute_index(pc, provider_table)]) != outcome) {
        if (alt_table != -1 && provider_table < tage_num_tables - 1) {
            // Find a table between alt_table + 1 and provider_table to allocate
            for (int i = provider_table + 1; i < tage_num_tables; i++) {
                uint32_t idx = tage_compute_index(pc, i);
                
                // Check useful bit to decide if we can replace
                if (i == tage_num_tables - 1 || tage_useful_bits[idx] == 0) {
                    tage_tag_tables[i][idx] = tage_compute_tag(pc, i);
                    tage_pred_counters[i][idx] = (outcome == TAKEN) ? WT : WN;
                    
                    if (i < tage_num_tables - 1) {
                        tage_useful_bits[idx] = 0;
                    }
                    break;
                }
            }
        }
    }
    
    // Update useful bits
    if (provider_table != -1 && alt_table != -1) {
        uint32_t provider_idx = tage_compute_index(pc, provider_table);
        if (predict_2_bit(tage_pred_counters[provider_table][provider_idx]) != 
            predict_2_bit(tage_pred_counters[alt_table][tage_compute_index(pc, alt_table)])) {
            
            if (predict_2_bit(tage_pred_counters[provider_table][provider_idx]) == outcome) {
                // Provider was correct
                if (tage_useful_bits[provider_idx] < 3) {
                    tage_useful_bits[provider_idx]++;
                }
            } else {
                // Provider was wrong
                if (tage_useful_bits[provider_idx] > 0) {
                    tage_useful_bits[provider_idx]--;
                }
            }
        }
    }
    
    // Update global history
    tage_ghistory = ((tage_ghistory << 1) | outcome) & ((1ULL << tage_ghistoryBits) - 1);
}

// Helper functions for TAGE
uint32_t tage_compute_index(uint32_t pc, int table_idx) {
    uint32_t history_mask = (1 << (tage_ghistoryBits - table_idx)) - 1;
    uint32_t folded_history = tage_fold_history(tage_ghistory & history_mask, table_idx);
    return (pc ^ folded_history) & (tage_table_sizes[table_idx] - 1);
}

uint8_t tage_compute_tag(uint32_t pc, int table_idx) {
    uint32_t tag = (pc >> 2) & ((1 << tage_tag_widths[table_idx]) - 1);
    uint32_t history_mask = (1 << (tage_ghistoryBits - table_idx)) - 1;
    uint32_t folded_history = tage_fold_history(tage_ghistory & history_mask, tage_tag_widths[table_idx]);
    return (tag ^ folded_history) & ((1 << tage_tag_widths[table_idx]) - 1);
}

uint32_t tage_fold_history(uint64_t history, int output_bits) {
    uint32_t folded = 0;
    while (history) {
        folded ^= (history & ((1 << output_bits) - 1));
        history >>= output_bits;
    }
    return folded;
}

void cleanup_tage() {
    free(tage_base_table);
    for (int i = 0; i < tage_num_tables; i++) {
        free(tage_tag_tables[i]);
        free(tage_pred_counters[i]);
    }
    free(tage_tag_tables);
    free(tage_pred_counters);
    free(tage_useful_bits);
}


//helper functions

void saturating_add(uint8_t *counter, uint8_t max){
  if(*counter < max){
    (*counter)++;
  }
}

void saturating_sub(uint8_t *counter, uint8_t min){
  if(*counter > min){
    (*counter)--;
  }
}


