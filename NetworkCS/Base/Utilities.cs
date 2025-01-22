using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.Serialization.Formatters.Binary;
using System.Windows.Controls;

namespace NetworkCS.Base
{
    public static class ExpendMethod
    {
        public static ObservableCollection<T> ToObservableCollection<T>(this IEnumerable<T> source)
        {
            if (source == null)
                return new ObservableCollection<T>();

            return new ObservableCollection<T>(source);
        }
        public static ObservableCollection<T> AddRange<T>(this ObservableCollection<T> source, IEnumerable<T> targets)
        {
            foreach (var target in targets)
            {
                source.Add(target);
            }

            return source;

        }
        public static void RemoveRange<T>(this ObservableCollection<T> source, IEnumerable<T> target)
        {
            List<T> items = new List<T>(target);
            foreach (var item in items)
            {
                if (source.Contains(item))
                {
                    source.Remove(item);
                }
            }
        }
        public static void RemoveRange<T>(this List<T> source, IEnumerable<T> target)
        {
            foreach (var item in target)
            {
                if (target.Contains(item))
                {
                    source.Remove(item);
                }
            }
        }
        public static void RemoveRange<T>(this ObservableCollection<T> source, int from, int count)
        {
            var deletedcompleted = source.ToList().GetRange(from, count);
            var target = deletedcompleted.ToObservableCollection<T>();
            RemoveRange(source, target);
        }

        public static double ToDouble(this object value, double defaultValue = 0)
        {
            string strValue = value.ToString();

            if (string.IsNullOrWhiteSpace(strValue))
            {
                return defaultValue;
            }

            double temp = 0;
            bool isConvert = double.TryParse(strValue, out temp);
            if (isConvert)
            {
                return temp;
            }
            else
            {
                return defaultValue;
            }
        }

        public static double ToFloat(this object value, float defaultValue = 0)
        {
            string strValue = value.ToString();

            if (string.IsNullOrWhiteSpace(strValue))
            {
                return defaultValue;
            }

            float temp = 0;
            bool isConvert = float.TryParse(strValue, out temp);
            if (isConvert)
            {
                return temp;
            }
            else
            {
                return defaultValue;
            }
        }

        public static bool ToBool(this object value, bool defaultbool = false)
        {
            string strValue = value.ToString();

            if (string.IsNullOrWhiteSpace(strValue))
            {
                return defaultbool;
            }

            bool temp = false;
            bool isConvert = bool.TryParse(strValue, out temp);
            if (isConvert)
            {
                return temp;
            }
            else
            {
                return defaultbool;
            }
        }

        public static int ToInt(this object value, int defaultValue = 0)
        {
            string strValue = value.ToString();

            if (string.IsNullOrWhiteSpace(strValue))
            {
                return defaultValue;
            }

            int temp = 0;
            bool isConvert = int.TryParse(strValue, out temp);
            if (isConvert)
            {
                return temp;
            }
            else
            {
                return defaultValue;
            }
        }
        public static ulong ToUlong(this object value, ulong defaultValue = 0)
        {
            string strValue = value.ToString();

            if (string.IsNullOrWhiteSpace(strValue))
            {
                return defaultValue;
            }

            ulong temp = 0;
            bool isConvert = ulong.TryParse(strValue, out temp);
            if (isConvert)
            {
                return temp;
            }
            else
            {
                return defaultValue;
            }
        }
        public static long ToLong(this object value, int defaultValue = 0)
        {
            string strValue = value.ToString();

            if (string.IsNullOrWhiteSpace(strValue))
            {
                return defaultValue;
            }

            long temp = 0;
            bool isConvert = long.TryParse(strValue, out temp);
            if (isConvert)
            {
                return temp;
            }
            else
            {
                return defaultValue;
            }
        }
        public static string ToNumberic(this object value)
        {
            string strValue = value.ToString();


            if (strValue == null)
            {
                return "0";
            }


            string strResult = string.Empty;
            foreach (var item in strValue)
            {
                int temp = 0;
                bool isConvert = int.TryParse(item.ToString(), out temp);
                if (isConvert)
                {
                    strResult += item;

                }
                else if (item == '.' && !strResult.Contains('.'))
                {
                    strResult += item;
                }
            }

            return string.IsNullOrEmpty(strResult) ? "0" : strResult;
        }
        public static string ToNumberic(this object value, int round)
        {
            string strValue = value.ToString();
            string newValue = strValue.ToNumberic();
            string[] newValues = newValue.Split('.');
            if (round < 1)
            {
                return newValues[0];
            }

            if (newValues.Length > 1)
            {
                if (string.IsNullOrEmpty(newValues[1]) || newValues[1].Length < round)
                {
                    return newValue;
                }
                string newRound = newValues[1].Substring(0, round);
                return string.Format("{0}.{1}", newValues[0], newRound);
            }
            return newValue;
        }
        public static bool IsNumber(this object objNumber)
        {
            string returnValue;

            if (objNumber != null)
                returnValue = objNumber.ToString().Replace(" ", "");
            else
                return true;

            foreach (var digitCheck in returnValue.ToCharArray())
            {
                if ((!char.IsDigit(digitCheck)) && (digitCheck != '.') && (digitCheck != '-'))
                {
                    return false;
                }
            }
            return true;
        }
        public static float ToFloat(this string floatStr, float defaultFloat = 0.0f)
        {
            try
            {
                if (string.IsNullOrEmpty(floatStr))
                {
                    return defaultFloat;
                }

                float outValue;
                if (float.TryParse(floatStr, out outValue))
                {
                    return outValue;
                }
                else
                {
                    return defaultFloat;
                }
            }
            catch
            {
                return defaultFloat;
            }
        }

        public static double ToDouble(this string floatStr, double defaultDouble = 0.0)
        {
            try
            {
                if (string.IsNullOrEmpty(floatStr))
                {
                    return defaultDouble;
                }

                double outValue;
                if (double.TryParse(floatStr, out outValue))
                {
                    return outValue;
                }
                else
                {
                    return defaultDouble;
                }
            }
            catch
            {
                return defaultDouble;
            }
        }
        public static int ToInt(this string intStr, int defaultInt = 0)
        {
            try
            {
                if (string.IsNullOrEmpty(intStr))
                {
                    return defaultInt;
                }

                int outValue;
                if (int.TryParse(intStr, out outValue))
                {
                    return outValue;
                }
                else
                {
                    return defaultInt;
                }
            }
            catch
            {
                return defaultInt;
            }
        }

        public static bool ToBool(this string intStr, bool defaultbool = false)
        {
            try
            {
                if (string.IsNullOrEmpty(intStr))
                {
                    return defaultbool;
                }

                bool outValue;
                if (bool.TryParse(intStr, out outValue))
                {
                    return outValue;
                }
                else
                {
                    return defaultbool;
                }
            }
            catch
            {
                return defaultbool;
            }
        }
        public static void Invoke(this Control control, Action ramda)
        {
            if (control != null && ramda != null)
            {
                control.Dispatcher.Invoke(ramda);
            }

        }

        //public static object DeepClone(object obj)
        //{
        //    object objResult = null;
        //    using (MemoryStream ms = new MemoryStream())
        //    {
        //        BinaryFormatter bf = new BinaryFormatter();
        //        bf.Serialize(ms, obj);

        //        ms.Position = 0;
        //        objResult = bf.Deserialize(ms);
        //    }
        //    return objResult;
        //}

        public static double RoundUp(this double value, int decimals = 2)
        {
            return Math.Round(value, decimals);
        }

        public static void PrintBits(this BitArray ba)
        {
            for (int i = 0; i < ba.Count; i++)
            {
                Console.Write(ba[i] ? "1" : "0");
            }
            Console.WriteLine();
        }

        public static DateTime ToDate(this string source, bool returnNull = true)
        {
            try
            {
                DateTime rtnDateTime;
                var result = DateTime.TryParse(source, out rtnDateTime);
                if (!result)
                {
                    if (returnNull)
                    {
                        DateTime? emptyDatetime = new DateTime();
                        return emptyDatetime.Value;
                    }
                    else
                    {
                        return DateTime.Now;
                    }

                }

                return rtnDateTime;
            }
            catch
            {
                return DateTime.Now;
            }
        }

        public static DateTime ToDateTimeUtc(this string source, bool returnNull = true)
        {
            try
            {
                DateTime rtnDateTime;
                var result = DateTime.TryParse(source, out rtnDateTime);
                if (!result)
                {
                    if (returnNull)
                    {
                        DateTime? emptyDatetime = new DateTime();
                        return emptyDatetime.Value.ToUniversalTime();
                    }
                    else
                    {
                        return DateTime.Now;
                    }

                }

                return rtnDateTime;
            }
            catch
            {
                return DateTime.Now;
            }
        }

        public static DateTime? ToInfluxTimeLocal(this string source, bool isNano = false)
        {
            try
            {
                string[] formats = new string[2] { "yyyy-MM-ddTHH:mm:ssZ", "yyyy-MM-ddTHH:mm:ss.fffZ" };
                DateTime rtnDateTime = DateTime.ParseExact(source, formats, null, DateTimeStyles.None);

                return rtnDateTime;
            }
            catch
            {
                return null;
            }
        }

        public static K FindFirstKeyByValue<K, V>(this Dictionary<K, V> dict, V val)
        {
            return dict.FirstOrDefault(entry =>
                EqualityComparer<V>.Default.Equals(entry.Value, val)).Key;
        }

        public static K FindFirstKeyByValue<K, V>(this ConcurrentDictionary<K, V> dict, V val)
        {
            return dict.FirstOrDefault(entry =>
                EqualityComparer<V>.Default.Equals(entry.Value, val)).Key;
        }

        //// Dictionary 를 Deep Copy 할 때 사용 용도
        //public static TData CloneData<TData>(TData data) where TData : class
        //{
        //    if (data == null)
        //    {
        //        return null;
        //    }

        //    MemoryStream ms = new MemoryStream();

        //    BinaryFormatter xs = new BinaryFormatter();
        //    xs.Serialize(ms, data);

        //    ms.Position = 0;

        //    return (TData)xs.Deserialize(ms);
        //}
    }
}
