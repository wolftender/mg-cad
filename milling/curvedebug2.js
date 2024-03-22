const CURVE2 = [[0.00290084, 0.111403],[0.00211916, 0.111613],[0.0040346, 0.111814],[0.00596906, 0.112013],[0.00791312, 0.112207],[0.00986685, 0.112397],[0.0118302, 0.112583],[0.0138032, 0.112765],[0.0157857, 0.112943],[0.0177778, 0.113118],[0.0197794, 0.113288],[0.0217904, 0.113454],[0.0238108, 0.113616],[0.0258454, 0.113775],[0.0278883, 0.113929],[0.0299404, 0.11408],[0.0320016, 0.114226],[0.0340716, 0.114369],[0.0361506, 0.114508],[0.038238, 0.114643],[0.040334, 0.114773],[0.0424433, 0.114901],[0.04456, 0.115024],[0.0466845, 0.115143],[0.048817, 0.115259],[0.0509574, 0.11537],[0.0531052, 0.115478],[0.0552601, 0.115582],[0.0574222, 0.115682],[0.0595959, 0.115779],[0.0617754, 0.115872],[0.0639616, 0.115961],[0.0661539, 0.116046],[0.0683522, 0.116127],[0.0705562, 0.116205],[0.0727658, 0.116279],[0.0749805, 0.116349],[0.0772048, 0.116416],[0.0794331, 0.116479],[0.0816661, 0.116539],[0.0839033, 0.116595],[0.0861446, 0.116647],[0.0883895, 0.116695],[0.090638, 0.11674],[0.0928899, 0.116782],[0.0951446, 0.116819],[0.097402, 0.116853],[0.0996619, 0.116884],[0.101924, 0.116911],[0.104192, 0.116935],[0.106462, 0.116954],[0.108733, 0.116971],[0.111005, 0.116984],[0.113278, 0.116993],[0.115552, 0.116999],[0.117826, 0.117001],[0.1201, 0.116999],[0.122374, 0.116994],[0.124648, 0.116986],[0.126922, 0.116974],[0.129194, 0.116958],[0.131466, 0.116939],[0.133736, 0.116917],[0.136004, 0.11689],[0.138271, 0.11686],[0.140535, 0.116827],[0.142798, 0.11679],[0.145057, 0.11675],[0.147314, 0.116705],[0.149569, 0.116658],[0.151819, 0.116606],[0.154067, 0.116551],[0.15631, 0.116493],[0.15855, 0.11643],[0.160786, 0.116365],[0.163017, 0.116295],[0.165244, 0.116222],[0.167467, 0.116145],[0.169684, 0.116065],[0.171896, 0.11598],[0.174103, 0.115892],[0.176304, 0.115801],[0.1785, 0.115705],[0.180689, 0.115606],[0.182873, 0.115504],[0.18505, 0.115397],[0.187221, 0.115287],[0.189386, 0.115173],[0.191543, 0.115055],[0.19369, 0.114934],[0.19583, 0.114808],[0.197963, 0.11468],[0.200088, 0.114547],[0.202206, 0.114411],[0.204317, 0.11427],[0.206419, 0.114127],[0.208514, 0.113979],[0.210601, 0.113828],[0.21268, 0.113672],[0.214751, 0.113514],[0.216814, 0.113351],[0.218869, 0.113185],[0.220915, 0.113015],[0.222953, 0.112841],[0.224983, 0.112663],[0.227004, 0.112482],[0.229013, 0.112298],[0.231014, 0.11211],[0.233006, 0.111918],[0.23499, 0.111723],[0.236965, 0.111524],[0.238932, 0.111322],[0.240891, 0.111116],[0.242841, 0.110907],[0.244782, 0.110695],[0.246716, 0.110479],[0.248641, 0.110259],[0.251791, 0.109892],[0.251791, 0.109892],[0.253714, 0.109664],[0.255642, 0.109433],[0.257576, 0.109198],[0.259516, 0.108961],[0.26146, 0.108721],[0.26341, 0.108478],[0.265369, 0.108231],[0.267332, 0.107982],[0.269301, 0.10773],[0.271275, 0.107475],[0.273254, 0.107217],[0.275237, 0.106957],[0.277225, 0.106694],[0.279217, 0.106429],[0.281214, 0.106161],[0.283214, 0.105891],[0.285219, 0.105618],[0.287228, 0.105343],[0.289241, 0.105065],[0.291257, 0.104786],[0.29328, 0.104503],[0.295307, 0.104218],[0.297337, 0.103931],[0.29937, 0.103642],[0.301406, 0.103351],[0.303445, 0.103058],[0.305486, 0.102763],[0.30753, 0.102465],[0.309576, 0.102166],[0.311624, 0.101865],[0.313675, 0.101562],[0.315727, 0.101257],[0.317781, 0.10095],[0.319836, 0.100642],[0.321892, 0.100331],[0.32395, 0.100019],[0.326009, 0.0997054],[0.328069, 0.0993899],[0.330129, 0.0990728],[0.33219, 0.098754],[0.334252, 0.0984337],[0.336314, 0.0981117],[0.338376, 0.0977883],[0.340438, 0.0974633],[0.3425, 0.0971368],[0.344561, 0.0968088],[0.346622, 0.0964794],[0.348683, 0.0961485],[0.350743, 0.0958163],[0.352802, 0.0954826],[0.35486, 0.0951476],[0.356917, 0.0948113],[0.358972, 0.0944737],[0.361027, 0.0941348],[0.363079, 0.0937946],[0.36513, 0.0934532],[0.367179, 0.0931106],[0.369227, 0.0927668],[0.371272, 0.0924218],[0.373315, 0.0920757],[0.375355, 0.0917285],[0.377394, 0.0913802],[0.379429, 0.0910308],[0.381462, 0.0906804],[0.383492, 0.090329],[0.385519, 0.0899766],[0.387543, 0.0896233],[0.389564, 0.089269],[0.391581, 0.0889138],[0.393595, 0.0885578],[0.395601, 0.0882016],[0.397604, 0.0878445],[0.399604, 0.0874867],[0.401599, 0.0871281],[0.403591, 0.0867689],[0.405578, 0.0864089],[0.407562, 0.0860484],[0.40954, 0.0856873],[0.411515, 0.0853256],[0.413484, 0.0849635],[0.415449, 0.0846008],[0.417406, 0.0842385],[0.419358, 0.0838757],[0.421305, 0.0835126],[0.423247, 0.0831493],[0.425184, 0.0827857],[0.427116, 0.0824219],[0.429042, 0.0820581],[0.430963, 0.0816943],[0.432879, 0.0813305],[0.43479, 0.0809668],[0.436691, 0.0806041],[0.438587, 0.0802415],[0.440477, 0.0798794],[0.442363, 0.0795176],[0.444242, 0.0791564],[0.446116, 0.0787958],[0.447984, 0.0784359],[0.449847, 0.0780767],[0.4517, 0.0777192],[0.453548, 0.0773625],[0.455391, 0.0770069],[0.457228, 0.0766524],[0.45906, 0.0762991],[0.460886, 0.0759472],[0.462706, 0.0755967],[0.464521, 0.0752477],[0.466327, 0.0749012],[0.468128, 0.0745563],[0.469923, 0.0742132],[0.471714, 0.073872],[0.473499, 0.0735329],[0.47528, 0.073196],[0.477055, 0.0728614],[0.478825, 0.0725291],[0.480591, 0.0721993],[0.482348, 0.0718731],[0.484101, 0.0715493],[0.485849, 0.0712285],[0.487593, 0.0709105],[0.489334, 0.0705956],[0.491069, 0.070284],[0.492801, 0.0699756],[0.49453, 0.0696707],[0.496254, 0.0693693],[0.497975, 0.0690716],[0.499688, 0.0687786],[0.501407, 0.0684883],[0.503136, 0.0682025],[0.504875, 0.0679207],[0.506624, 0.0676431],[0.508384, 0.0673697],[0.510155, 0.0671005],[0.511937, 0.0668354],[0.51373, 0.0665746],[0.515534, 0.066318],[0.517349, 0.0660657],[0.519176, 0.0658175],[0.521013, 0.0655734],[0.522861, 0.0653335],[0.524726, 0.0650971],[0.5266, 0.0648649],[0.528485, 0.0646368],[0.530382, 0.0644126],[0.53229, 0.0641923],[0.534208, 0.0639758],[0.536137, 0.0637631],[0.538077, 0.0635541],[0.540027, 0.0633488],[0.541993, 0.0631464],[0.543968, 0.0629475],[0.545953, 0.0627521],[0.547947, 0.0625599],[0.549952, 0.0623708],[0.551966, 0.0621848],[0.553989, 0.0620017],[0.556027, 0.061821],[0.558072, 0.0616432],[0.560125, 0.0614679],[0.562188, 0.0612952],[0.564258, 0.0611249],[0.566336, 0.0609569],[0.568422, 0.0607911],[0.570521, 0.060627],[0.572625, 0.060465],[0.574736, 0.0603048],[0.576854, 0.0601464],[0.578979, 0.0599896],[0.581109, 0.0598345],[0.583246, 0.0596808],[0.585388, 0.0595285],[0.587541, 0.0593772],[0.589697, 0.0592273],[0.591858, 0.0590785],[0.594024, 0.0589307],[0.596194, 0.058784],[0.598368, 0.0586382],[0.600546, 0.0584932],[0.602728, 0.0583492],[0.604913, 0.058206],[0.607102, 0.0580635],[0.609298, 0.0579214],[0.611495, 0.0577802],[0.613695, 0.0576397],[0.615897, 0.0575],[0.618102, 0.0573613],[0.620308, 0.0572235],[0.622516, 0.0570869],[0.624727, 0.0569516],[0.626938, 0.0568177],[0.629151, 0.0566853],[0.631365, 0.0565547],[0.633581, 0.0564259],[0.635797, 0.0562992],[0.638014, 0.0561747],[0.640232, 0.0560526],[0.642451, 0.0559332],[0.64467, 0.0558167],[0.646889, 0.0557033],[0.649109, 0.0555932],[0.651328, 0.0554866],[0.653548, 0.0553839],[0.655766, 0.0552854],[0.657985, 0.0551912],[0.660203, 0.0551018],[0.66242, 0.0550174],[0.664636, 0.0549383],[0.666851, 0.054865],[0.669065, 0.0547977],[0.671277, 0.0547367],[0.673486, 0.0546826],[0.675694, 0.0546355],[0.677898, 0.0545959],[0.6801, 0.0545642],[0.682299, 0.0545407],[0.684493, 0.0545259],[0.686684, 0.0545201],[0.68887, 0.0545237],[0.691051, 0.0545371],[0.693222, 0.0545606],[0.695388, 0.0545946],[0.697547, 0.0546394],[0.699699, 0.0546953],[0.701843, 0.0547627],[0.703978, 0.0548417],[0.706104, 0.0549328],[0.708221, 0.0550361],[0.710326, 0.0551516],[0.712421, 0.0552797],[0.714504, 0.0554205],[0.716574, 0.0555739],[0.718627, 0.0557398],[0.720667, 0.0559184],[0.722692, 0.0561098],[0.724702, 0.0563138],[0.726697, 0.0565304],[0.728676, 0.0567595],[0.730639, 0.0570009],[0.732584, 0.0572543],[0.734513, 0.0575196],[0.736424, 0.0577965],[0.738318, 0.0580848],[0.740193, 0.0583841],[0.742051, 0.0586941],[0.743887, 0.0590139],[0.745706, 0.0593435],[0.747506, 0.0596828],[0.749289, 0.0600313],[0.751056, 0.0603885],[0.752816, 0.0607542],[0.754571, 0.0611276],[0.756321, 0.0615084],[0.758066, 0.0618962],[0.759805, 0.0622905],[0.761541, 0.0626908],[0.763275, 0.0630976],[0.765006, 0.0635098],[0.766733, 0.063927],[0.768457, 0.0643488],[0.770178, 0.0647749],[0.771896, 0.065205],[0.773612, 0.0656387],[0.775326, 0.0660758],[0.777038, 0.0665161],[0.778748, 0.0669592],[0.780458, 0.0674049],[0.782166, 0.0678529],[0.783874, 0.0683031],[0.785582, 0.0687553],[0.78729, 0.0692091],[0.788998, 0.0696645],[0.790706, 0.0701213],[0.792415, 0.0705792],[0.794126, 0.0710382],[0.795837, 0.0714981],[0.79755, 0.0719586],[0.799265, 0.0724198],[0.800981, 0.0728814],[0.8027, 0.0733433],[0.804421, 0.0738055],[0.806144, 0.0742677],[0.80787, 0.0747299],[0.809599, 0.0751919],[0.811331, 0.0756537],[0.813066, 0.0761152],[0.814804, 0.0765763],[0.816549, 0.0770376],[0.818298, 0.0774984],[0.82005, 0.0779585],[0.821806, 0.0784179],[0.823566, 0.0788764],[0.825331, 0.0793341],[0.827099, 0.0797908],[0.828872, 0.0802464],[0.830649, 0.0807009],[0.83243, 0.0811543],[0.834217, 0.0816064],[0.836007, 0.0820573],[0.837803, 0.0825068],[0.839603, 0.0829549],[0.841408, 0.0834016],[0.843218, 0.0838467],[0.845033, 0.0842903],[0.846853, 0.0847322],[0.848678, 0.0851725],[0.850508, 0.0856111],[0.852342, 0.086048],[0.854182, 0.086483],[0.856028, 0.0869162],[0.857878, 0.0873475],[0.859733, 0.0877769],[0.861594, 0.0882043],[0.86346, 0.0886296],[0.86533, 0.0890529],[0.867206, 0.089474],[0.869087, 0.0898931],[0.870973, 0.0903099],[0.872864, 0.0907244],[0.874761, 0.0911367],[0.876662, 0.0915466],[0.878568, 0.0919542],[0.880479, 0.0923594],[0.882394, 0.092762],[0.884315, 0.0931622],[0.88624, 0.0935599],[0.88817, 0.0939549],[0.890105, 0.0943473],[0.892044, 0.094737],[0.893987, 0.095124],[0.895935, 0.0955083],[0.897887, 0.0958898],[0.899843, 0.0962684],[0.901803, 0.0966444],[0.903766, 0.0970174],[0.905733, 0.0973877],[0.907703, 0.0977551],[0.909677, 0.0981196],[0.911653, 0.0984813],[0.913633, 0.09884],[0.91561, 0.0991952],[0.917591, 0.0995475],[0.919575, 0.0998969],[0.92156, 0.100243],[0.923548, 0.100587],[0.925537, 0.100927],[0.927528, 0.101265],[0.92952, 0.101599],[0.931513, 0.10193],[0.933507, 0.102259],[0.935501, 0.102584],[0.937497, 0.102906],[0.939492, 0.103225],[0.941488, 0.103541],[0.943479, 0.103853],[0.94547, 0.104162],[0.947461, 0.104468],[0.949452, 0.10477],[0.951442, 0.10507],[0.95343, 0.105366],[0.955418, 0.105658],[0.957403, 0.105948],[0.959388, 0.106234],[0.96137, 0.106517],[0.963346, 0.106796],[0.965321, 0.107071],[0.967293, 0.107343],[0.969262, 0.107612],[0.97123, 0.107877],[0.973194, 0.108139],[0.975156, 0.108397],[0.977114, 0.108652],[0.979069, 0.108903],[0.981017, 0.10915],[0.982962, 0.109393],[0.984903, 0.109633],[0.986841, 0.109869],[0.988775, 0.110102],[0.990706, 0.110331],[0.992632, 0.110556],[0.994555, 0.110777],[0.996474, 0.110994],[0.998389, 0.111208],];