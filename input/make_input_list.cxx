

void make_input_list() {

  ofstream f;

  f.open("eff_vs_bkg.txt");

  double eff_u = 0.1;
  double bkg_u = 0.025;

  for (int i = 0; i < 10; i++) {
    double eff = (i + 1) * eff_u;
    for (int j = 0; j < 10; j++) {
      double bkg = (j + 1) * bkg_u;
      f << i << "_" << j << " 4 0.03 " << eff << " 0.06 " << bkg << " 0.25\n";
    }
  }
  f.close();
}
