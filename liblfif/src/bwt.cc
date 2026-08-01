#include <vector>
#include <cstdlib>
#include <map>
#include <algorithm>

const bool S_TYPE = false;
const bool L_TYPE = true;

std::vector<bool> buildTypeMap(const std::vector<int64_t> &string) {
  std::vector<bool> result(string.size() + 1);

  result[result.size() - 1] = S_TYPE;

  if (string.size() == 0) {
    return result;
  }

  result[result.size() - 2] = L_TYPE;

  for (int64_t i = string.size() - 2; i >= 0; i--) {
    if (string[i] > string[i + 1]) {
      result[i] = L_TYPE;
    }
    else if (string[i] < string[i + 1]) {
      result[i] = S_TYPE;
    }
    else {
      result[i] = result[i + 1];
    }
  }

  return result;
}


bool isLmsChar(const std::vector<bool> &typemap, size_t offset) {
  if (offset == 0) {
    return false;
  }

  if (typemap[offset] == S_TYPE && typemap[offset - 1] == L_TYPE) {
    return true;
  }

  return false;
}

bool lmsSubstringsAreEqual(const std::vector<int64_t> &string, const std::vector<bool> &typemap, size_t offset_A, size_t offset_B) {
  if (offset_A == string.size() || offset_B == string.size()) {
    return false;
  }

  size_t i = 0;
  while (true) {
    bool is_lms_A = isLmsChar(typemap, i + offset_A);
    bool is_lms_B = isLmsChar(typemap, i + offset_B);

    if (i != 0 && is_lms_A && is_lms_B) {
      return true;
    }

    if (is_lms_A != is_lms_B) {
      return false;
    }

    if (string[i + offset_A] != string[i + offset_B]) {
      return false;
    }

    i++;
  }
}

std::map<int64_t, size_t> findBucketSizes(const std::vector<int64_t> &string) {
  std::map<int64_t, size_t> result {};

  for (size_t i = 0; i < string.size(); i++) {
    result[string[i]]++;
  }

  return result;
}

std::map<int64_t, size_t> findBucketHeads(const std::map<int64_t, size_t> &bucket_sizes) {
  std::map<int64_t, size_t> result {};
  size_t offset = 1;
  for (const auto &pair: bucket_sizes) {
    result[pair.first] = offset;
    offset += pair.second;
  }

  return result;
}

std::map<int64_t, size_t> findBucketTails(const std::map<int64_t, size_t> &bucket_sizes) {
  std::map<int64_t, size_t> result {};
  size_t offset = 1;
  for (const auto &pair: bucket_sizes) {
    offset += pair.second;
    result[pair.first] = offset - 1;
  }

  return result;
}

std::vector<int64_t> guessLmsSort(const std::vector<int64_t> &string, const std::map<int64_t, size_t> &bucket_sizes, const std::vector<bool> &typemap) {
  std::vector<int64_t> guessed_suffix_array(string.size() + 1);
  std::ranges::fill(guessed_suffix_array, -1);

  std::map<int64_t, size_t> bucket_tails = findBucketTails(bucket_sizes);

  for (size_t i = 0; i < string.size(); i++) {
    if (!isLmsChar(typemap, i)) {
      continue;
    }

    int64_t bucket_index = string[i];
    guessed_suffix_array[bucket_tails[bucket_index]] = i;
    bucket_tails[bucket_index]--;
  }

  guessed_suffix_array[0] = string.size();

  return guessed_suffix_array;
}

void induceSortL(const std::vector<int64_t> &string, std::vector<int64_t> &guessed_suffix_array, const std::map<int64_t, size_t> &bucket_sizes, const std::vector<bool> &typemap) {
  std::map<int64_t, size_t> bucket_heads = findBucketHeads(bucket_sizes);

  for (size_t i = 0; i < guessed_suffix_array.size(); i++) {
    if (guessed_suffix_array[i] == -1) {
      continue;
    }

    int64_t j = guessed_suffix_array[i] - 1;

    if (j < 0) {
      continue;
    }

    if (typemap[j] != L_TYPE) {
      continue;
    }

    int64_t bucket_index = string[j];
    guessed_suffix_array[bucket_heads[bucket_index]] = j;
    bucket_heads[bucket_index]++;
  }
}

void induceSortS(const std::vector<int64_t> &string, std::vector<int64_t> &guessed_suffix_array, const std::map<int64_t, size_t> &bucket_sizes, const std::vector<bool> &typemap) {
  std::map<int64_t, size_t> bucket_tails = findBucketTails(bucket_sizes);

  for (int64_t i = guessed_suffix_array.size() - 1; i >= 0; i--) {
    int64_t j = guessed_suffix_array[i] - 1;

    if (j < 0) {
      continue;
    }

    if (typemap[j] != S_TYPE) {
      continue;
    }

    int64_t bucket_index = string[j];
    guessed_suffix_array[bucket_tails[bucket_index]] = j;
    bucket_tails[bucket_index]--;
  }
}

void summariseSuffixArray(const std::vector<int64_t> &string, std::vector<int64_t> &guessed_suffix_array, const std::vector<bool> &typemap, std::vector<int64_t> &summary_string, int64_t &summary_alphabet_size, std::vector<size_t> &summary_suffix_offsets) {
  std::vector<int64_t> lms_names(string.size() + 1);
  std::ranges::fill(lms_names, -1);

  int64_t current_name = 0;

  lms_names[guessed_suffix_array[0]] = current_name;
  int64_t last_lms_suffix_offset = guessed_suffix_array[0];

  for (size_t i = 1; i < guessed_suffix_array.size(); i++) {
    int64_t suffix_offset = guessed_suffix_array[i];

    if (!isLmsChar(typemap, suffix_offset)) {
      continue;
    }

    if (!lmsSubstringsAreEqual(string, typemap, last_lms_suffix_offset, suffix_offset)) {
      current_name++;
    }

    last_lms_suffix_offset = suffix_offset;

    lms_names[suffix_offset] = current_name;
  }

  summary_string.clear();
  summary_suffix_offsets.clear();

  for (size_t i = 0; i < lms_names.size(); i++) {
    if (lms_names[i] == -1) {
      continue;
    }

    summary_suffix_offsets.push_back(i);
    summary_string.push_back(lms_names[i]);
  }

  summary_alphabet_size = current_name + 1;
}

std::vector<int64_t> accurateLmsSort(const std::vector<int64_t> &string, const std::map<int64_t, size_t> &bucket_sizes, const std::vector<int64_t> &summary_suffix_array, const std::vector<size_t> &summary_suffix_offsets) {
  std::vector<int64_t> suffix_offsets(string.size() + 1);
  std::ranges::fill(suffix_offsets, -1);

  std::map<int64_t, size_t> bucket_tails = findBucketTails(bucket_sizes);

  for (int64_t i = summary_suffix_array.size() - 1; i > 1; i--) {
    size_t string_index = summary_suffix_offsets[summary_suffix_array[i]];
    int64_t bucket_index = string[string_index];
    suffix_offsets[bucket_tails[bucket_index]] = string_index;
    bucket_tails[bucket_index]--;
  }

  suffix_offsets[0] = string.size();

  return suffix_offsets;
}

std::vector<int64_t> makeSummarySuffixArray(const std::vector<int64_t> &summary_string, size_t summary_alphabet_size);

std::vector<int64_t> makeSuffixArrayByInducedSorting(const std::vector<int64_t> &string) {
  std::vector<bool> typemap = buildTypeMap(string);
  std::map<int64_t, size_t> bucket_sizes = findBucketSizes(string);
  std::vector<int64_t> guessed_suffix_array = guessLmsSort(string, bucket_sizes, typemap);

  induceSortL(string, guessed_suffix_array, bucket_sizes, typemap);
  induceSortS(string, guessed_suffix_array, bucket_sizes, typemap);

  std::vector<int64_t> summary_string {};
  int64_t summary_alphabet_size {};
  std::vector<size_t> summary_suffix_offsets {};

  summariseSuffixArray(string, guessed_suffix_array, typemap, summary_string, summary_alphabet_size, summary_suffix_offsets);

  std::vector<int64_t> summary_suffix_array = makeSummarySuffixArray(summary_string, summary_alphabet_size);

  std::vector<int64_t> accurate_suffix_array = accurateLmsSort(string, bucket_sizes, summary_suffix_array, summary_suffix_offsets);

  induceSortL(string, accurate_suffix_array, bucket_sizes, typemap);
  induceSortS(string, accurate_suffix_array, bucket_sizes, typemap);

  return accurate_suffix_array;
}

std::vector<int64_t> makeSummarySuffixArray(const std::vector<int64_t> &summary_string, size_t summary_alphabet_size) {
  if (summary_alphabet_size == summary_string.size()) {
    std::vector<int64_t> summary_suffix_array(summary_string.size() + 1);

    summary_suffix_array[0] = summary_string.size();

    for (size_t i = 0; i < summary_string.size(); i++) {
      int64_t j = summary_string[i];
      summary_suffix_array[j + 1] = i;
    }

    return summary_suffix_array;
  }
  else {
    return makeSuffixArrayByInducedSorting(summary_string);
  }
}

template<typename OI, typename VI>
void reorder(OI order_begin, OI order_end, VI values_begin) {
  typedef typename std::iterator_traits<VI>::value_type value_t;
  typedef typename std::iterator_traits<OI>::value_type index_t;
  typedef typename std::iterator_traits<OI>::difference_type diff_t;

  diff_t n = order_end - order_begin;

  for (diff_t i {}; i < n; i++) {
    if (i != order_begin[i]) {
      value_t tA = values_begin[i];
      diff_t j = i;

      index_t k = order_begin[j];
      while (i != k) {
        values_begin[j] = values_begin[k];
        order_begin[j] = j;
        j = k;
        k = order_begin[j];
      }

      values_begin[j] = tA;
      order_begin[j] = j;
    }
  }
}

size_t bwt(std::vector<int64_t> &string) {
  std::vector<int64_t> suffix_array = makeSuffixArrayByInducedSorting(string);

  size_t pidx {};
  for (size_t i = 1; i < suffix_array.size(); i++) {
    if (suffix_array[i] == 0) {
      pidx = i - 1;
      suffix_array[i] += string.size();
    }

    suffix_array[i]--;
  }

  reorder(std::begin(suffix_array) + 1, std::end(suffix_array), std::begin(string));

  return pidx;
}


void ibwt(std::vector<int64_t> &string, size_t pidx) {
  std::vector<std::pair<int64_t, size_t>> F(string.size());

  for (size_t i = 0; i < F.size(); i++) {
    F[i] = {string[i], i};
  }

  std::ranges::sort(F);

  std::vector<size_t> T(F.size());
  for (size_t i = 0; i < F.size(); i++) {
    T[F[i].second] = i;
  }

  std::vector<int64_t> S(F.size());
  for (size_t i = 0; i < S.size(); i++) {
    S[S.size() - i - 1] = string[pidx];
    pidx = T[pidx];
  }

  string = S;
}
