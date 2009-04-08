#include "GibblerMaxTransDecoder.h"

#include <sstream>
#include <map>

using namespace __gnu_cxx;
using namespace std;
using namespace Josiah;

namespace Josiah
{


  template<class M>
  void MaxCollector<M>::getDistribution(map<const M*,float>& p) const
  {
    const vector<float>& importanceWeights =  getImportanceWeights();
    const M* prev = NULL;
    for (typename map<M,vector<size_t> >::const_iterator i = m_samples.begin(); i != m_samples.end(); ++i) {
      const M* sample = &(i->first);
      for (vector<size_t>::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
        p[sample] += importanceWeights[*j];
      }
    }
    IFVERBOSE(1) {
      float total = 0;
      VERBOSE(1, "Distribution: ");
      for (typename map<const M*,float>::const_iterator i = p.begin(); i != p.end(); ++i) {
        VERBOSE(1, i->first << "{ " << *i->first << " }: " <<  i->second << " " << endl;);
        total += i->second;
      }
      VERBOSE(1, endl << "Total = " << total << endl);
    }
  }
  
  template<class M>
  float MaxCollector<M>::getEntropy() const
  {
    map<const M*, float> p;
    getDistribution(p);
    float entropy = 0;
    //cerr << "Entropy: ";
    for (typename map<const M*,float>::const_iterator pi = p.begin(); pi != p.end(); ++pi) {
      //cerr << pi->second << " ";
      entropy -= pi->second*log(pi->second);
    }
    //cerr << endl;
    //cerr << "Entropy : " << entropy << endl;
    return entropy;
  }

  template<class M>
  void MaxCollector<M>::collectSample( const M &m)
  {
    m_samples[m].push_back(N());
    typename map<M,vector<size_t> >::const_iterator i = m_samples.find(m);
    m_sampleList.push_back(&(i->first));
  }
  
  template<class M>
      const M* MaxCollector<M>::getSample(size_t index) const 
  {
    return m_sampleList.at(index);
  }
  
  template<class M>
      pair<const M*,float> MaxCollector<M>::getMax() const 
  {
    const M* argmax = NULL;
    float max = 0;
    map<const M*,float> p;
    getDistribution(p);
    for (typename map<const M*,float>::const_iterator pi = p.begin(); pi != p.end(); ++pi) {
      if (pi->second > max) {
        max = pi->second;
        argmax = pi->first;
      }
    }
    
    return pair<const M*,float>(argmax,max);
  }
  
  template<class M>
  struct ProbGreaterThan :  public std::binary_function<const pair<const M*,float>&,const pair<const M*,float>&,bool>{
    bool operator()(const pair<const M*,float>& d1, const pair<const M*,float>& d2) const {
      return d1.second > d2.second; 
    }
  };
  
  template<class M>
      void MaxCollector<M>::getNbest(vector<pair<const M*, float> >& nbest, size_t n) const 
  {
    map<const M*,float> p;
    getDistribution(p);
    nbest.assign(p.begin(),p.end());
    ProbGreaterThan<M> comparator;

    stable_sort(nbest.begin(),nbest.end(),comparator);
    while (nbest.size() > n) {
      nbest.pop_back();
    }
  }

  template class MaxCollector<Josiah::Derivation>;
  template class MaxCollector<Josiah::Translation>;

  GibblerMaxTransDecoder::GibblerMaxTransDecoder() : n(0), m_outputMaxChange(false) {}



  string ToString(const Translation& ws)
  {
    ostringstream os;
    for (Translation::const_iterator i = ws.begin(); i != ws.end(); ++i)
      os << (*i)->GetString() << " ";
    return os.str();
  }

  ostream& operator<<(ostream& out, const Translation& ws)
  {
    out << ToString(ws);
    return out;
  }

  void GibblerMaxTransDecoder::collect(Sample& sample)
  {
    ++n;
    const Hypothesis* h = sample.GetSampleHypothesis();
    vector<const Factor*> trans;
    h->GetTranslation(&trans, 0);
    ++samples[trans];

    if (m_outputMaxChange) {
      Translation newmax;
      size_t count;
      Max(newmax,count);
      if (newmax != m_maxTranslation) {
        m_maxTranslation = newmax;
        cerr << "NewMaxTrans(" << n << ") ";
        cerr << ToString(m_maxTranslation);
        cerr << endl;
      }
    }
  }

  void GibblerMaxTransDecoder::Max(std::vector<const Factor*>& translation, size_t& count)
  {
    hash_map<vector<const Factor*>, int>::const_iterator ci;
    multimap<float, const vector<const Factor*>*,greater<float> > sorted;
    const float nf = n;
    for (ci = samples.begin(); ci != samples.end(); ++ci) {
      sorted.insert(make_pair<float, const vector<const Factor*>*>(static_cast<float>(ci->second) / nf, &ci->first));
    }
    multimap<float, const vector<const Factor*>*,greater<float> >::iterator i;
    for (i = sorted.begin(); i != sorted.end(); ++i)
      VERBOSE(1, i->first << "\t" << ToString(*i->second) << endl);
    const vector<const Factor*>* max = sorted.begin()->second;
    translation.insert(translation.end(), max->begin(), max->end());
    count = (size_t)samples[*max];

  }

  void GibblerMaxTransDecoder::getCounts( std::vector< size_t > & counts ) const
  {
    hash_map<vector<const Factor*>, int>::const_iterator ci;
    for (ci = samples.begin(); ci != samples.end(); ++ci) {
      counts.push_back((size_t)ci->second);
    }
  }



}




